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
cc_pp_iter(FILE *f, const struct fsm_options *opt,
	struct re_char_class_ast *n);

static void
pp_iter(FILE *f, const struct fsm_options *opt, struct ast_expr *n);

static int
atomic(struct ast_expr *n)
{
	switch (n->t) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_LITERAL:
	case AST_EXPR_ANY:
	case AST_EXPR_CHAR_CLASS:
	case AST_EXPR_GROUP:
		return 1;

	case AST_EXPR_REPEATED:
	case AST_EXPR_CONCAT:
	case AST_EXPR_ALT:
		return 0;

	case AST_EXPR_FLAGS:
		return 0; /* XXX */

	default:
		assert(0);
	}
}

static void
print_grouped(FILE *f, const struct fsm_options *opt, struct ast_expr *n)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(n != NULL);

	if (atomic(n)) {
		pp_iter(f, opt, n);
	} else {
		fprintf(f, "(");
		pp_iter(f, opt, n);
		fprintf(f, ")");
	}
}

static void
pp_repeat(FILE *f, const struct fsm_options *opt, struct ast_expr *n,
	unsigned m)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(n != NULL);

	if (m == 0) {
		return;
	}

	if (m == 1) {
		pp_iter(f, opt, n);
		return;
	}

	print_grouped(f, opt, n);
	fprintf(f, "{%u}", m);
}

static void
pp_iter(FILE *f, const struct fsm_options *opt, struct ast_expr *n)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (n == NULL) { return; }

	switch (n->t) {
	case AST_EXPR_EMPTY:
		break;

	case AST_EXPR_CONCAT:
		pp_iter(f, opt, n->u.concat.l);
		fprintf(f, " ");
		pp_iter(f, opt, n->u.concat.r);
		break;

	case AST_EXPR_CONCAT_N:
	{
		size_t i;
		for (i = 0; i < n->u.concat_n.count; i++) {
			pp_iter(f, opt, n->u.concat_n.n[i]);
			if (i < n->u.concat_n.count - 1) { fprintf(f, " "); }
		}
		break;
	}

	case AST_EXPR_ALT:
		pp_iter(f, opt, n->u.alt.l);
		fprintf(f, " | "); /* XXX: indent */
		pp_iter(f, opt, n->u.alt.r);
		break;

	case AST_EXPR_ALT_N:
	{
		size_t i;
		for (i = 0; i < n->u.alt_n.count; i++) {
			pp_iter(f, opt, n->u.alt_n.n[i]);
			if (i < n->u.alt_n.count - 1) {
				fprintf(f, " | "); /* XXX: indent */
			}
		}
		break;
	}

	/* TODO: concat a sequence of literals to "xyz" */
	case AST_EXPR_LITERAL:
		fprintf(f, "\"");
		blab_escputc(f, opt, n->u.literal.c);
		fprintf(f, "\"");
		break;

	case AST_EXPR_ANY:
		fprintf(f, "any");
		break;

	case AST_EXPR_REPEATED: {
		if (n->u.repeated.low == 0 && n->u.repeated.high == AST_COUNT_UNBOUNDED) {
			print_grouped(f, opt, n->u.repeated.e);
			fprintf(f, "*");
			return;
		}

		if (n->u.repeated.low == 1 && n->u.repeated.high == AST_COUNT_UNBOUNDED) {
			print_grouped(f, opt, n->u.repeated.e);
			fprintf(f, "+");
			return;
		}

		if (n->u.repeated.low == 0 && n->u.repeated.high == 1) {
			pp_iter(f, opt, n->u.repeated.e);
			fprintf(f, "?");
			return;
		}

		if (n->u.repeated.low == n->u.repeated.high) {
			pp_repeat(f, opt, n->u.repeated.e, n->u.repeated.low);
			return;
		}

		print_grouped(f, opt, n->u.repeated.e);
		fprintf(f, "{%u,%u}", n->u.repeated.low, n->u.repeated.high);

		break;
	}

	case AST_EXPR_CHAR_CLASS:
		fprintf(f, "[");
		cc_pp_iter(f, opt, n->u.char_class.cca);
		fprintf(f, "]");
		break;

	case AST_EXPR_GROUP:
		fprintf(f, "(");
		pp_iter(f, opt, n->u.group.e);
		fprintf(f, ")");
		break;

	case AST_EXPR_ANCHOR:
		assert(n->u.anchor.t == RE_AST_ANCHOR_START
		    || n->u.anchor.t == RE_AST_ANCHOR_END);
		fprintf(f, "%s", n->u.anchor.t == RE_AST_ANCHOR_START ? "START" : "END");
		break;

	case AST_EXPR_FLAGS:
		abort();

	default:
		assert(0);
	}
}

void
re_ast_print_blab(FILE *f, const struct fsm_options *opt,
	const struct ast_re *ast)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(ast != NULL);

	fprintf(f, "e = ");

	pp_iter(f, opt, ast->expr);

	fprintf(f, "\n");
}

static void
cc_pp_iter(FILE *f, const struct fsm_options *opt, struct re_char_class_ast *n)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(n != NULL);

	switch (n->t) {
	case RE_CHAR_CLASS_AST_CONCAT:
if (n->u.concat.l != NULL && n->u.concat.l->t == RE_CHAR_CLASS_AST_FLAGS) {
	/* XXX */
	cc_pp_iter(f, opt, n->u.concat.r);
} else {
		cc_pp_iter(f, opt, n->u.concat.l);
		cc_pp_iter(f, opt, n->u.concat.r);
}
		break;

	case RE_CHAR_CLASS_AST_LITERAL:
		blab_escputc(f, opt, n->u.literal.c);
		break;

	case RE_CHAR_CLASS_AST_RANGE: {
		int c;

		if (n->u.range.from.t != AST_RANGE_ENDPOINT_LITERAL || n->u.range.to.t != AST_RANGE_ENDPOINT_LITERAL) {
			fprintf(stderr, "non-literal range endpoint unsupported\n");
			abort(); /* XXX */
		}

		blab_escputc(f, opt, n->u.range.from.u.literal.c);
		fprintf(f, "-");
		blab_escputc(f, opt, n->u.range.to.u.literal.c);
		break;
	}

	case RE_CHAR_CLASS_AST_NAMED:
		fprintf(f, "todo"); /* XXX */
		break;

	case RE_CHAR_CLASS_AST_FLAGS:
		if (n->u.flags.f & RE_CHAR_CLASS_FLAG_INVERTED) {
			fprintf(f, "SOL");
		}
		if (n->u.flags.f & RE_CHAR_CLASS_FLAG_MINUS) {
			fprintf(f, "'-'"); /* XXX */
		}
		break;

	case RE_CHAR_CLASS_AST_SUBTRACT:
		fprintf(stderr, "subtract unsupported\n");
		abort(); /* XXX */

		cc_pp_iter(f, opt, n->u.subtract.ast);
		cc_pp_iter(f, opt, n->u.subtract.mask);
		break;

	default:
		fprintf(stderr, "(MATCH FAIL)\n");
		assert(0);
	}
}

