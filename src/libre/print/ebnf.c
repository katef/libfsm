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

	fprintf(f, "%u * ", m);
	print_grouped(f, opt, n);
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
		fprintf(f, ", ");
		pp_iter(f, opt, n->u.concat.r);
		break;

	case AST_EXPR_ALT:
		pp_iter(f, opt, n->u.alt.l);
		fprintf(f, " | "); /* XXX: indent */
		pp_iter(f, opt, n->u.alt.r);
		break;

	case AST_EXPR_LITERAL:
		fprintf(f, "\'");
		ebnf_escputc(f, opt, n->u.literal.c);
		fprintf(f, "\'");
		break;

	case AST_EXPR_ANY:
		fprintf(f, "any");
		break;

	case AST_EXPR_REPEATED: {
		unsigned delta;
		unsigned i;

		if (n->u.repeated.high == AST_COUNT_UNBOUNDED) {
			pp_repeat(f, opt, n->u.repeated.e, n->u.repeated.low);
			if (n->u.repeated.low > 0) {
				fprintf(f, ", ");
			}

			fprintf(f, "{ ");
			pp_iter(f, opt, n->u.repeated.e);
			fprintf(f, " }");

			return;
		}

		if (n->u.repeated.low == 0 && n->u.repeated.high == 1) {
			fprintf(f, "[ ");
			pp_iter(f, opt, n->u.repeated.e);
			fprintf(f, " ]");

			return;
		}

		pp_repeat(f, opt, n->u.repeated.e, n->u.repeated.low);

		delta = n->u.repeated.high - n->u.repeated.low;

		if (n->u.repeated.low > 0 && delta > 0) {
			fprintf(f, ", ");
		}

		for (i = 0; i < delta; i++) {
			fprintf(f, "[ ");
			pp_iter(f, opt, n->u.repeated.e);

			if (i + 1 < delta) {
				fprintf(f, ", ");
			}
		}
		for (i = 0; i < delta; i++) {
			fprintf(f, " ]");
		}

		break;
	}

	case AST_EXPR_CHAR_CLASS:
		fprintf(f, "(");
		cc_pp_iter(f, opt, n->u.char_class.cca);
		fprintf(f, ")");
		break;

	case AST_EXPR_GROUP:
		fprintf(f, "(");
		pp_iter(f, opt, n->u.group.e);
		fprintf(f, ")");
		break;

	case AST_EXPR_FLAGS:
		abort();

	default:
		assert(0);
	}
}

void
re_ast_print_ebnf(FILE *f, const struct fsm_options *opt,
	const struct ast_re *ast)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(ast != NULL);

	fprintf(f, "e = ");

	pp_iter(f, opt, ast->expr);

	fprintf(f, "\n");
	fprintf(f, "  ;");
	fprintf(f, "\n");
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
		fprintf(f, " | ");
		cc_pp_iter(f, opt, n->u.concat.r);
}
		break;

	case RE_CHAR_CLASS_AST_LITERAL:
		fprintf(f, "\'");
		ebnf_escputc(f, opt, n->u.literal.c);
		fprintf(f, "\'");
		break;

	case RE_CHAR_CLASS_AST_RANGE: {
		int c;

		if (n->u.range.from.t != AST_RANGE_ENDPOINT_LITERAL || n->u.range.to.t != AST_RANGE_ENDPOINT_LITERAL) {
			fprintf(stderr, "non-literal range endpoint unsupported\n");
			abort(); /* XXX */
		}

		for (c = n->u.range.from.u.literal.c; c <= n->u.range.to.u.literal.c; c++) {
			fprintf(f, "\'");
			ebnf_escputc(f, opt, c);
			fprintf(f, "\'");

			if (c + 1 <= n->u.range.to.u.literal.c) {
				fprintf(f, " | ");
			}
		}
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

