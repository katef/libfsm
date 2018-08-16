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

static int
atomic(struct ast_expr *n)
{
	switch (n->t) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_LITERAL:
	case AST_EXPR_ANY:
	case AST_EXPR_REPEATED:
	case AST_EXPR_CHAR_CLASS:
	case AST_EXPR_GROUP:
		return 1;

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
		pp_iter(f, opt, n->u.concat.r);
		break;

	case AST_EXPR_CONCAT_N:
	{
		size_t i;
		for (i = 0; i < n->u.concat_n.count; i++) {
			pp_iter(f, opt, n->u.concat_n.n[i]);
		}
		break;
	}

	case AST_EXPR_ALT:
		pp_iter(f, opt, n->u.alt.l);
		fprintf(f, "|");
		pp_iter(f, opt, n->u.alt.r);
		break;

	case AST_EXPR_ALT_N:
	{
		size_t i;
		for (i = 0; i < n->u.alt_n.count; i++) {
			pp_iter(f, opt, n->u.alt_n.n[i]);
			if (i < n->u.alt_n.count - 1) {
				fprintf(f, "|");
			}
		}
		break;
	}

	case AST_EXPR_LITERAL:
		pcre_escputc(f, opt, n->u.literal.c);
		break;

	case AST_EXPR_ANY:
		fprintf(f, ".");
		break;

	case AST_EXPR_REPEATED: {
		size_t i;

		static const struct {
			unsigned m;
			unsigned n;
			const char *op;
		} a[] = {
#define _ AST_COUNT_UNBOUNDED
			{ 0, 0, NULL },
			{ 0, _, "*"  },
			{ 0, 1, "?"  },
			{ 1, _, "+"  },
			{ 1, 1, NULL }
#undef _
		};

		if (atomic(n->u.repeated.e)) {
			pp_iter(f, opt, n->u.repeated.e);
		} else {
			fprintf(f, "(?:");
			pp_iter(f, opt, n->u.repeated.e);
			fprintf(f, ")");
		}

		assert(n->u.repeated.high == AST_COUNT_UNBOUNDED || n->u.repeated.high >= n->u.repeated.low);

		for (i = 0; i < sizeof a / sizeof *a; i++) {
			if (a[i].m == n->u.repeated.low && a[i].n == n->u.repeated.high) {
				assert(a[i].op != NULL);
				fprintf(f, "%s", a[i].op);
				break;
			}
		}

		if (i == sizeof a / sizeof *a) {
			fprintf(f, "{");
			fprintf(f, "%u", n->u.repeated.low);
			if (n->u.repeated.high == n->u.repeated.low) {
				/* nothing */
			} else if (n->u.repeated.high == AST_COUNT_UNBOUNDED) {
				fprintf(f, ",");
			} else {
				fprintf(f, ",");
				fprintf(f, "%u", n->u.repeated.high);
			}
			fprintf(f, "}");
		}

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
		fprintf(f, "%s", n->u.anchor.t == RE_AST_ANCHOR_START ? "^" : "$");
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
re_ast_print_pcre(FILE *f, const struct fsm_options *opt,
	const struct ast_re *ast)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(ast != NULL);

	pp_iter(f, opt, ast->expr);

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
print_range_endpoint(FILE *f, const struct fsm_options *opt,
	const struct ast_range_endpoint *r)
{
	switch (r->t) {
	case AST_RANGE_ENDPOINT_LITERAL:
		pcre_escputc(f, opt, r->u.literal.c);
		break;

	default:
		assert(0);
		break;
	}
}

static void
cc_pp_iter(FILE *f, const struct fsm_options *opt, struct re_char_class_ast *n)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(n != NULL);

	switch (n->t) {
	case RE_CHAR_CLASS_AST_CONCAT:
		cc_pp_iter(f, opt, n->u.concat.l);
		cc_pp_iter(f, opt, n->u.concat.r);
		break;

	case RE_CHAR_CLASS_AST_LITERAL:
		pcre_escputc(f, opt, n->u.literal.c);
		break;

	case RE_CHAR_CLASS_AST_RANGE:
		print_range_endpoint(f, opt, &n->u.range.from);
		fprintf(f, "-");
		print_range_endpoint(f, opt, &n->u.range.to);
		break;

	case RE_CHAR_CLASS_AST_NAMED:
		fprintf(f, "[:TODO:]"); /* XXX */
		break;

	case RE_CHAR_CLASS_AST_FLAGS:
		if (n->u.flags.f & RE_CHAR_CLASS_FLAG_INVERTED) {
			fprintf(f, "^");
		}
		if (n->u.flags.f & RE_CHAR_CLASS_FLAG_MINUS) {
			fprintf(f, "-");
		}
		break;

	case RE_CHAR_CLASS_AST_SUBTRACT:
		fprintf(f, "\tn%p [ label = <{CLASS-SUBTRACT|{ast|mask}}> ];\n", (void *) n);
		cc_pp_iter(f, opt, n->u.subtract.ast);
		cc_pp_iter(f, opt, n->u.subtract.mask);
		break;

	default:
		fprintf(stderr, "(MATCH FAIL)\n");
		assert(0);
	}
}

