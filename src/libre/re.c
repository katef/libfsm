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
#include "print.h"

#include "re_ast.h"
#include "re_comp.h"
#include "re_analysis.h"

#include "ac.h"

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
		{ RE_LIKE,    parse_re_like,    0, RE_ANCHORED },
		{ RE_LITERAL, parse_re_literal, 0, RE_ANCHORED },
		{ RE_GLOB,    parse_re_glob,    0, RE_ANCHORED },
		{ RE_NATIVE,  parse_re_native,  0, 0           },
		{ RE_PCRE,    parse_re_pcre,    0, 0           },
		{ RE_SQL,     parse_re_sql,     1, RE_ANCHORED }
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

struct ast_re *
re_parse(enum re_dialect dialect, int (*getc)(void *opaque), void *opaque,
	const struct fsm_options *opt,
	enum re_flags flags, struct re_err *err)
{
	const struct dialect *m;
	struct ast_re *ast = NULL;
	enum re_analysis_res res;
	
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

	/* Do a complete pass over the AST, filling in other details. */
	res = re_ast_analysis(ast);

	if (res < 0) {
		re_ast_free(ast);
		if (err != NULL) { err->e = RE_EERRNO; }
		return NULL;
	}

	ast->unsatisfiable = (res == RE_ANALYSIS_UNSATISFIABLE);

	return ast;
}

struct fsm *
re_comp(enum re_dialect dialect, int (*getc)(void *opaque), void *opaque,
	const struct fsm_options *opt,
	enum re_flags flags, struct re_err *err)
{
	struct ast_re *ast;
	struct fsm *new;
	const struct dialect *m;

	m = re_dialect(dialect);

	if (m == NULL) {
		if (err != NULL) { err->e = RE_EBADDIALECT; }
		return NULL;
	}

	flags |= m->flags;

	ast = re_parse(dialect, getc, opaque, opt, flags, err);
	if (ast == NULL) { return NULL; }

	/* If the RE is inherently unsatisfiable, then free the
	 * AST and replace it with an empty tombstone node.
	 * This will compile to an FSM that matches nothing, so
	 * that unioning it with other regexes will still work. */
	if (ast->unsatisfiable) {
		struct ast_expr *unsat = ast->expr;
		ast->expr = re_ast_expr_tombstone();
		re_ast_expr_free(unsat);
	}

	new = re_comp_ast(ast, flags, opt, err);
	re_ast_free(ast);
	ast = NULL;

	if (new == NULL) {
		if (err != NULL && err->e == RE_ESUCCESS) {
			/* If we got here, we had a parse error
			 * without error information set. */
			assert(0);
		}
		return NULL;
	}
	
	/*
	 * All flags operators commute with respect to composition.
	 * That is, the order of application here does not matter;
	 * here I'm trying to keep these ordered for efficiency.
	 */

	if (flags & RE_REVERSE) {
		if (!fsm_reverse(new)) { goto error; }
	}

	return new;

error:

	if (new != NULL) { fsm_free(new); }
	if (ast != NULL) { re_ast_free(ast); }
	if (err != NULL) { err->e = RE_EERRNO; }

	return NULL;
}

static int
re_strings_endstate(struct fsm *fsm, struct fsm_state **endp, enum re_strings_flags flags)
{
	struct fsm_state *end = NULL;
	int sym;

	if ((flags & RE_STRINGS_AC_AUTOMATON) || (flags & RE_STRINGS_ANCHOR_RIGHT)) {
		goto finish;
	}

	end = fsm_addstate(fsm);
	if (end == NULL) {
		goto error;
	}

	for (sym=0; sym <= UCHAR_MAX; sym++) {
		if (!fsm_addedge_literal(fsm, end, end, sym)) {
			goto error;
		}
	}

finish:
	if (end != NULL) {
		fsm_setend(fsm, end, 1);
	}

	*endp = end;
	return 1;

error:
	*endp = NULL;
	if (end != NULL) {
		fsm_removestate(fsm,end);
	}

	return 0;
}

struct fsm *
re_strings(const struct fsm_options *opts, const char *sv[], size_t n, enum re_strings_flags flags)
{
	struct trie_graph *g;
	struct fsm *fsm;
	struct fsm_state *end;
	size_t i;
	size_t c = 0;
	size_t cn = n/100;

	g = trie_create();
	fsm = NULL;

	for (i=0; i < n; i++) {
		size_t len;

		len = strlen(sv[i]);
		if (!trie_add_word(g, sv[i], len)) {
			goto finish;
		}

		if (++c == cn) {
			c=0;
		}
	}

	if ((flags & RE_STRINGS_ANCHOR_LEFT) == 0) {
		if (trie_add_failure_edges(g) < 0) {
			goto finish;
		}
	}

	fsm = fsm_new(opts);
	if (fsm == NULL) {
		goto finish;
	}

	end = NULL;
	if (!re_strings_endstate(fsm, &end, flags)) {
		fsm_free(fsm);
		goto finish;
	}

	if (!trie_to_fsm(fsm, g, end)) {
		fsm_free(fsm);
		goto finish;
	}

finish:
	trie_free(g);
	return fsm;
}

