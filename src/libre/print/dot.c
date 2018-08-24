/*
 * Copyright 2018 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <stdio.h>
#include <ctype.h>

#include <print/esc.h>

#include "../re_ast.h"
#include "../re_char_class.h"
#include "../print.h"

static void
re_flags_print(FILE *f, enum re_flags fl);

static void
cc_pp_iter(FILE *f, const struct fsm_options *opt,
	const void *parent, struct re_char_class_ast *n);

static void
print_range_endpoint(FILE *f, const struct fsm_options *opt,
	const struct ast_range_endpoint *r);

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
pp_iter(FILE *f, const struct fsm_options *opt,
	const void *parent, struct ast_expr *n)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (n == NULL) { return; }

	if (parent != NULL) {
		fprintf(f, "\tn%p -> n%p;\n", parent, (void *) n);
	}

	switch (n->t) {
	case AST_EXPR_EMPTY:
		fprintf(f, "\tn%p [ label = <EMPTY> ];\n", (void *) n);
		break;

	case AST_EXPR_CONCAT:
		fprintf(f, "\tn%p [ label = <CONCAT> ];\n", (void *) n);
		pp_iter(f, opt, n, n->u.concat.l);
		pp_iter(f, opt, n, n->u.concat.r);
		break;

	case AST_EXPR_CONCAT_N:
	{
		size_t i;
		fprintf(f, "\tn%p [ label = <CONCAT|%lu> ];\n",
		    (void *) n, n->u.concat_n.count);
		for (i = 0; i < n->u.concat_n.count; i++) {
			pp_iter(f, opt, n, n->u.concat_n.n[i]);
		}
		break;
	}

	case AST_EXPR_ALT:
		fprintf(f, "\tn%p [ label = <ALT> ];\n", (void *) n);
		pp_iter(f, opt, n, n->u.alt.l);
		pp_iter(f, opt, n, n->u.alt.r);
		break;

	case AST_EXPR_ALT_N:
	{
		size_t i;
		fprintf(f, "\tn%p [ label = <ALT|%lu> ];\n",
		    (void *) n, n->u.alt_n.count);
		for (i = 0; i < n->u.alt_n.count; i++) {
			pp_iter(f, opt, n, n->u.alt_n.n[i]);
		}
		break;
	}

	case AST_EXPR_LITERAL:
		fprintf(f, "\tn%p [ label = <LITERAL|", (void *) n);
		dot_escputc_html(f, opt, n->u.literal.c);
		fprintf(f, "> ];\n");
		break;

	case AST_EXPR_ANY:
		fprintf(f, "\tn%p [ label = <ANY> ];\n", (void *) n);
		break;

	case AST_EXPR_REPEATED:
		fprintf(f, "\tn%p [ label = <REPEATED|&#x7b;", (void *) n);
		fprintf_count(f, n->u.repeated.low);
		fprintf(f, ", ");
		fprintf_count(f, n->u.repeated.high);
		fprintf(f, "&#x7d;> ];\n");
		pp_iter(f, opt, n, n->u.repeated.e);
		break;

	case AST_EXPR_CHAR_CLASS:
		fprintf(f, "\tn%p [ label = <CHAR_CLASS> ];\n", (void *) n);
		cc_pp_iter(f, opt, n, n->u.char_class.cca);
		break;

	case AST_EXPR_GROUP:
		fprintf(f, "\tn%p [ label = <GROUP|#%u> ];\n", (void *) n,
			n->u.group.id);
		pp_iter(f, opt, n, n->u.group.e);
		break;

	case AST_EXPR_ANCHOR:
		assert(n->u.anchor.t == RE_AST_ANCHOR_START ||
		    n->u.anchor.t == RE_AST_ANCHOR_END);
		fprintf(f, "\tn%p [ label = <ANCHOR|%c> ];\n",
		    (void *) n,
		    n->u.anchor.t == RE_AST_ANCHOR_START ? '^' : '$');
		break;

	case AST_EXPR_FLAGS:
		fprintf(f, "\tn%p [ label = <{FLAGS|{+", (void *) n);
		re_flags_print(f, n->u.flags.pos);
		fprintf(f, "|-");
		re_flags_print(f, n->u.flags.neg);
		fprintf(f, "> ];\n");
		break;

	default:
		assert(0);
	}
}

void
re_ast_print_dot(FILE *f, const struct fsm_options *opt,
	const struct ast_re *ast)
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

static void
re_flags_print(FILE *f, enum re_flags fl)
{
	const char *sep = "";
	if (fl & RE_ICASE) { fprintf(f, "%si", sep); sep = " "; }
	if (fl & RE_TEXT) { fprintf(f, "%sg", sep); sep = " "; }
	if (fl & RE_MULTI) { fprintf(f, "%sm", sep); sep = " "; }
	if (fl & RE_REVERSE) { fprintf(f, "%sr", sep); sep = " "; }
	if (fl & RE_SINGLE) { fprintf(f, "%ss", sep); sep = " "; }
	if (fl & RE_ZONE) { fprintf(f, "%sz", sep); sep = " "; }
}

static void
print_range_endpoint(FILE *f, const struct fsm_options *opt,
	const struct ast_range_endpoint *r)
{
	switch (r->t) {
	case AST_RANGE_ENDPOINT_LITERAL:
		dot_escputc_html(f, opt, r->u.literal.c);
		break;

	default:
		assert(0);
		break;
	}
}

static void
cc_pp_iter(FILE *f, const struct fsm_options *opt,
	const void *parent, struct re_char_class_ast *n)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(n != NULL);

	if (parent != NULL) {
		fprintf(f, "\tn%p -> n%p;\n", parent, (void *) n);
	}

	fprintf(f, "\tn%p [ style = \"filled\", fillcolor = \"#eeeeee\" ];\n", (void *) n);

	switch (n->t) {
	case RE_CHAR_CLASS_AST_CONCAT:
		fprintf(f, "\tn%p [ label = <CLASS-CONCAT> ];\n", (void *) n);
		cc_pp_iter(f, opt, n, n->u.concat.l);
		cc_pp_iter(f, opt, n, n->u.concat.r);
		break;

	case RE_CHAR_CLASS_AST_LITERAL:
		fprintf(f, "\tn%p [ label = <CLASS-LITERAL|", (void *) n);
		dot_escputc_html(f, opt, n->u.literal.c);
		fprintf(f, "> ];\n");
		break;

	case RE_CHAR_CLASS_AST_RANGE:
		fprintf(f, "\tn%p [ label = <CLASS-RANGE|", (void *) n);
		print_range_endpoint(f, opt, &n->u.range.from);
		fprintf(f, " &ndash; ");
		print_range_endpoint(f, opt, &n->u.range.to);
		fprintf(f, "> ];\n");
		break;

	case RE_CHAR_CLASS_AST_NAMED:
		fprintf(f, "\tn%p [ label = <CLASS-NAMED|(opaque)> ];\n", (void *) n);
		break;

	case RE_CHAR_CLASS_AST_FLAGS:
		fprintf(f, "\tn%p [ label = <CLASS-FLAGS|", (void *) n);
		if (n->u.flags.f & RE_CHAR_CLASS_FLAG_INVERTED) {
			fprintf(f, "^");
		}
		if (n->u.flags.f & RE_CHAR_CLASS_FLAG_MINUS) {
			fprintf(f, "-");
		}
		fprintf(f, "> ];\n");
		break;

	case RE_CHAR_CLASS_AST_SUBTRACT:
		fprintf(f, "\tn%p [ label = <{CLASS-SUBTRACT|{ast|mask}}> ];\n", (void *) n);
		cc_pp_iter(f, opt, n, n->u.subtract.ast);
		cc_pp_iter(f, opt, n, n->u.subtract.mask);
		break;

	default:
		fprintf(stderr, "(MATCH FAIL)\n");
		assert(0);
	}
}

