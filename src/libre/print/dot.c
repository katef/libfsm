/*
 * Copyright 2018 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

#include <fsm/fsm.h>

#include <re/re.h>

#include <print/esc.h>

#include "../class.h"
#include "../ast.h"
#include "../print.h"

static void
fprintf_count(FILE *f, unsigned count)
{
	if (count == AST_COUNT_UNBOUNDED) {
		fprintf(f, "&#x221e;");
	} else {
		fprintf(f, "%u", count);
	}
}

static void
re_flags_print(FILE *f, enum re_flags fl)
{
	if (fl & RE_ICASE  ) { fprintf(f, "i"); }
	if (fl & RE_TEXT   ) { fprintf(f, "g"); }
	if (fl & RE_MULTI  ) { fprintf(f, "m"); }
	if (fl & RE_REVERSE) { fprintf(f, "r"); }
	if (fl & RE_SINGLE ) { fprintf(f, "s"); }
	if (fl & RE_ZONE   ) { fprintf(f, "z"); }
}

static void
print_endpoint(FILE *f, const struct fsm_options *opt, const struct ast_endpoint *e)
{
	switch (e->type) {
	case AST_ENDPOINT_LITERAL:
		dot_escputc_html_record(f, opt, e->u.literal.c);
		break;

	case AST_ENDPOINT_CODEPOINT:
		fprintf(f, "U+%lX", (unsigned long) e->u.codepoint.u);
		break; 

	default:
		assert(!"unreached");
		break;
	}
}

static void
pp_iter(FILE *f, const struct fsm_options *opt,
	const void *parent, struct ast_expr *n)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (n == NULL) { return; }

	if (parent != NULL) {
		fprintf(f, "\tn%p -> n%p;\n", parent, (void *) n);
	}

	switch (n->type) {
	case AST_EXPR_EMPTY:
		fprintf(f, "\tn%p [ label = <EMPTY> ];\n", (void *) n);
		break;

	case AST_EXPR_CONCAT:
	{
		size_t i;
		fprintf(f, "\tn%p [ label = <CONCAT|%lu> ];\n",
		    (void *) n, n->u.concat.count);
		for (i = 0; i < n->u.concat.count; i++) {
			pp_iter(f, opt, n, n->u.concat.n[i]);
		}
		break;
	}

	case AST_EXPR_ALT:
	{
		size_t i;
		fprintf(f, "\tn%p [ label = <ALT|%lu> ];\n",
		    (void *) n, n->u.alt.count);
		for (i = 0; i < n->u.alt.count; i++) {
			pp_iter(f, opt, n, n->u.alt.n[i]);
		}
		break;
	}

	case AST_EXPR_LITERAL:
		fprintf(f, "\tn%p [ label = <LITERAL|", (void *) n);
		dot_escputc_html_record(f, opt, n->u.literal.c);
		fprintf(f, "> ];\n");
		break;

	case AST_EXPR_CODEPOINT:
		fprintf(f, "\tn%p [ label = <CODEPOINT|U+%lX> ];\n", (void *) n,
			(unsigned long) n->u.codepoint.u);
		break; 

	case AST_EXPR_ANY:
		fprintf(f, "\tn%p [ label = <ANY> ];\n", (void *) n);
		break;

	case AST_EXPR_REPEAT:
		fprintf(f, "\tn%p [ label = <REPEAT|&#x7b;", (void *) n);
		fprintf_count(f, n->u.repeat.min);
		fprintf(f, ", ");
		fprintf_count(f, n->u.repeat.max);
		fprintf(f, "&#x7d;> ];\n");
		pp_iter(f, opt, n, n->u.repeat.e);
		break;

	case AST_EXPR_GROUP:
		fprintf(f, "\tn%p [ label = <GROUP|#%u> ];\n", (void *) n,
			n->u.group.id);
		pp_iter(f, opt, n, n->u.group.e);
		break;

	case AST_EXPR_ANCHOR:
		assert(n->u.anchor.type == AST_ANCHOR_START || n->u.anchor.type == AST_ANCHOR_END);
		fprintf(f, "\tn%p [ label = <ANCHOR|%c> ];\n",
		    (void *) n,
		    n->u.anchor.type == AST_ANCHOR_START ? '^' : '$');
		break;

	case AST_EXPR_SUBTRACT:
		fprintf(f, "\tn%p [ label = <{SUBTRACT|{a|b}}> ];\n", (void *) n);
		pp_iter(f, opt, n, n->u.subtract.a);
		pp_iter(f, opt, n, n->u.subtract.b);
		break;

	case AST_EXPR_RANGE:
		fprintf(f, "\tn%p [ label = <RANGE|", (void *) n);
		print_endpoint(f, opt, &n->u.range.from);
		fprintf(f, " &ndash; ");
		print_endpoint(f, opt, &n->u.range.to);
		fprintf(f, "> ];\n");
		break;

	case AST_EXPR_FLAGS:
		fprintf(f, "\tn%p [ label = <FLAGS|{+", (void *) n);
		re_flags_print(f, n->u.flags.pos);
		fprintf(f, "}|{-");
		re_flags_print(f, n->u.flags.neg);
		fprintf(f, "}> ];\n");
		break;

	case AST_EXPR_TOMBSTONE:
		fprintf(f, "\tn%p [ label = <RIP> ];\n", (void *) n);
		break;

	default:
		assert(!"unreached");
	}
}

void
ast_print_dot(FILE *f, const struct fsm_options *opt,
	const struct ast *ast)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(ast != NULL);

	fprintf(f, "digraph G {\n");
	fprintf(f, "\tnode [ shape = Mrecord ];\n");
	fprintf(f, "\tedge [ dir = none ];\n");
	fprintf(f, "\troot = n%p;\n", (void *) ast);
	fprintf(f, "\tsplines = false;\n");
	fprintf(f, "\n");

	pp_iter(f, opt, NULL, ast->expr);

	fprintf(f, "}\n");
}

