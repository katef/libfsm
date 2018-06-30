/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <re/re.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>

#include "../libfsm/internal.h" /* XXX */

#include "dialect/comp.h"

#include "re_ast.h"
#include "re_print.h"
#include "re_comp.h"
#include "re_analysis.h"

struct dialect {
	enum re_dialect dialect;
	re_dialect_parse_fun *parse;
	int overlap;
};

static const struct dialect *
re_dialect(enum re_dialect dialect)
{
	size_t i;

	static const struct dialect a[] = {
		{ RE_LIKE,       parse_re_like,    0 },
		{ RE_LITERAL,    parse_re_literal, 0 },
		{ RE_GLOB,       parse_re_glob,    0 },
		{ RE_NATIVE,     parse_re_native,  0 },
		{ RE_PCRE,       parse_re_pcre,    0 },
		{ RE_SQL,        parse_re_sql,     1 }
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
			*f &= ~RE_ANCHOR;
		}

		switch (*p) {
		case 'i': *f |= RE_ICASE;   break;
		case 'g': *f |= RE_TEXT;    break;
		case 'm': *f |= RE_MULTI;   break;
		case 'r': *f |= RE_REVERSE; break;
		case 's': *f |= RE_SINGLE;  break;
		case 'z': *f |= RE_ZONE;    break;

		default:
			errno = EINVAL;
			return -1;
		}
	}

	return 0;
}

struct ast_re *
re_parse(enum re_dialect dialect, int (*getc)(void *opaque), void *opaque,
	const struct fsm_options *opt,
	enum re_flags flags, struct re_err *err)
{
	const struct dialect *m;
	struct ast_re *ast;

	assert(getc != NULL);

	m = re_dialect(dialect);
	if (m == NULL) {
		if (err != NULL) {
			err->e = RE_EBADDIALECT;
		}
		return NULL;
	}

	ast = m->parse(getc, opaque, opt, flags, m->overlap, err);
	if (ast == NULL) {
		return NULL;
	}

	/* Do a complete pass over the AST, filling in other details. */
	re_ast_analysis(ast);

	/* TODO: this should be a CLI flag or something */
	if (PRETTYPRINT_AST) {
		re_ast_print(stderr, ast);
	}

	return ast;
}

struct fsm *
re_comp(enum re_dialect dialect, int (*getc)(void *opaque), void *opaque,
	const struct fsm_options *opt,
	enum re_flags flags, struct re_err *err)
{
	struct ast_re *ast;
	struct fsm *new;

	ast = re_parse(dialect, getc, opaque, opt, flags, err);
	if (ast == NULL) {
		return NULL;
	}

	new = re_comp_ast(ast, flags, opt);
	re_ast_free(ast);

	if (new == NULL) {
		fprintf(stderr, "Compilation failed\n");
		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXUNSUPPORTD; /* FIXME */
		}
		return NULL;
	}
	
	/*
	 * All flags operators commute with respect to composition.
	 * That is, the order of application here does not matter;
	 * here I'm trying to keep these ordered for efficiency.
	 */

	if (flags & RE_REVERSE) {
		if (!fsm_reverse(new)) {
			goto error;
		}
	}

	return new;

error:

	fsm_free(new);

	if (err != NULL) {
		err->e = RE_EERRNO;
	}

	return NULL;
}

