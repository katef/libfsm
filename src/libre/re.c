/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <re/re.h>

#include <fsm/fsm.h>

#include "ac.h"
#include "class.h"
#include "print.h"
#include "ast.h"
#include "ast_analysis.h"
#include "ast_compile.h"

#include "dialect/comp.h"

struct dialect {
	enum re_dialect dialect;
	re_dialect_parse_fun *parse;
	int overlap;
	enum re_flags flags; /* ever-present flags which cannot be disabled */
};

static const struct dialect *
re_dialect(enum re_dialect dialect)
{
	size_t i;

	static const struct dialect a[] = {
		{ RE_LIKE,    parse_re_like,    0, RE_SINGLE | RE_ANCHORED },
		{ RE_LITERAL, parse_re_literal, 0, RE_SINGLE | RE_ANCHORED },
		{ RE_GLOB,    parse_re_glob,    0, RE_SINGLE | RE_ANCHORED },
		{ RE_NATIVE,  parse_re_native,  0, 0                       },
		{ RE_PCRE,    parse_re_pcre,    0, RE_END_NL               },
		{ RE_SQL,     parse_re_sql,     1, RE_SINGLE | RE_ANCHORED }
	};

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (a[i].dialect == dialect) {
			return &a[i];
		}
	}

	return NULL;
}

int
re_flags(const char *s, enum re_flags *f)
{
	const char *p;

	assert(s != NULL);
	assert(f != NULL);

	*f = 0U;

	/* defaults */
	*f |= RE_ZONE;

	for (p = s; *p != '\0'; p++) {
		if (*p & RE_ANCHOR) {
			*f &= (unsigned)~RE_ANCHOR;
		}

		switch (*p) {
		case 'a': *f |= RE_ANCHORED; break; /* PCRE's /x/A flag means /^x/ only */
		case 'i': *f |= RE_ICASE;    break;
		case 'g': *f |= RE_TEXT;     break;
		case 'm': *f |= RE_MULTI;    break;
		case 'r': *f |= RE_REVERSE;  break;
		case 's': *f |= RE_SINGLE;   break;
		case 'z': *f |= RE_ZONE;     break;

		default:
			errno = EINVAL;
			return -1;
		}
	}

	return 0;
}

struct ast *
re_parse(enum re_dialect dialect, int (*getc)(void *opaque), void *opaque,
	const struct fsm_options *opt,
	enum re_flags flags, struct re_err *err, int *unsatisfiable)
{
	const struct dialect *m;
	struct ast *ast = NULL;
	enum ast_analysis_res res;
	
	assert(getc != NULL);

	m = re_dialect(dialect);
	if (m == NULL) {
		if (err != NULL) { err->e = RE_EBADDIALECT; }
		return NULL;
	}

	flags |= m->flags;

	ast = m->parse(getc, opaque, opt, flags, m->overlap, err);

	if (ast == NULL) {
		return NULL;
	}

	if (!ast_rewrite(ast, flags)) {
		ast_free(ast);
		return NULL;
	}

	/* Do a complete pass over the AST, filling in other details. */
	res = ast_analysis(ast, flags);

	if (res < 0) {
		ast_free(ast);
		if (err != NULL) { err->e = RE_EERRNO; }
		return NULL;
	}

	if (unsatisfiable != NULL) {
		*unsatisfiable = (res == AST_ANALYSIS_UNSATISFIABLE);
	}

	return ast;
}

struct fsm *
re_comp(enum re_dialect dialect, int (*getc)(void *opaque), void *opaque,
	const struct fsm_options *opt,
	enum re_flags flags, struct re_err *err)
{
	struct ast *ast;
	struct fsm *new;
	const struct dialect *m;
	int unsatisfiable;

	m = re_dialect(dialect);
	if (m == NULL) {
		if (err != NULL) { err->e = RE_EBADDIALECT; }
		return NULL;
	}

	flags |= m->flags;

	ast = re_parse(dialect, getc, opaque, opt, flags, err, &unsatisfiable);
	if (ast == NULL) {
		return NULL;
	}

	/*
	 * If the RE is inherently unsatisfiable, then free the
	 * AST and replace it with an empty tombstone node.
	 * This will compile to an FSM that matches nothing, so
	 * that unioning it with other regexes will still work.
	 */
	if (unsatisfiable) {
		ast_expr_free(ast->pool, ast->expr);
		/* ast_free below frees the pool */

		ast->expr = ast_expr_tombstone;
	}

	new = ast_compile(ast, flags, opt, err);

	ast_free(ast);

	if (new == NULL) {
		/* XXX: this can happen e.g. on malloc failure */
		assert(err == NULL || err->e != RE_ESUCCESS);
		goto error;
	}

	return new;

error:

	if (err != NULL) {
		err->e = RE_EERRNO;
	}

	return NULL;
}
