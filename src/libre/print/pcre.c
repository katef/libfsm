/*
 * Copyright 2018 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include <re/re.h>

#include <print/esc.h>

#include "../class.h"
#include "../class_lookup.h"
#include "../ast.h"
#include "../print.h"

static int
atomic(struct ast_expr *n)
{
	switch (n->type) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_LITERAL:
	case AST_EXPR_ANY:
	case AST_EXPR_REPEAT:
	case AST_EXPR_GROUP:
	case AST_EXPR_TOMBSTONE:
		return 1;

	case AST_EXPR_FLAGS:
		return 0; /* XXX */

	default:
		assert(!"unreached");
		abort();
	}
}

static void
re_flags_print(FILE *f, enum re_flags fl)
{
	const char *sep = "";

	if (fl & RE_ICASE  ) { fprintf(f, "%si", sep); sep = " "; }
	if (fl & RE_TEXT   ) { fprintf(f, "%sg", sep); sep = " "; }
	if (fl & RE_MULTI  ) { fprintf(f, "%sm", sep); sep = " "; }
	if (fl & RE_REVERSE) { fprintf(f, "%sr", sep); sep = " "; }
	if (fl & RE_SINGLE ) { fprintf(f, "%ss", sep); sep = " "; }
	if (fl & RE_ZONE   ) { fprintf(f, "%sz", sep); sep = " "; }
}

static void
print_endpoint(FILE *f, const struct fsm_options *opt, const struct ast_endpoint *e)
{
	switch (e->type) {
	case AST_ENDPOINT_LITERAL:
		pcre_escputc(f, opt, e->u.literal.c);
		break;

	case AST_ENDPOINT_CODEPOINT:
		fprintf(f, "\\x{%lX}", (unsigned long) e->u.codepoint.u);
		break;

	default:
		assert(!"unreached");
		break;
	}
}

static void
pp_iter(FILE *f, const struct fsm_options *opt, struct ast_expr *n)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (n == NULL) { return; }

	switch (n->type) {
	case AST_EXPR_EMPTY:
		break;

	case AST_EXPR_CONCAT:
	{
		size_t i;
		for (i = 0; i < n->u.concat.count; i++) {
			pp_iter(f, opt, n->u.concat.n[i]);
		}
		break;
	}

	case AST_EXPR_ALT:
	{
		size_t i;
		for (i = 0; i < n->u.alt.count; i++) {
			pp_iter(f, opt, n->u.alt.n[i]);
			if (i + 1 < n->u.alt.count) {
				fprintf(f, "|");
			}
		}
		break;
	}

	case AST_EXPR_LITERAL:
		pcre_escputc(f, opt, n->u.literal.c);
		break;

	case AST_EXPR_CODEPOINT:
		fprintf(f, "\\x{%lX}", (unsigned long) n->u.codepoint.u);
		break;

	case AST_EXPR_ANY:
		fprintf(f, ".");
		break;

	case AST_EXPR_REPEAT: {
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

		if (atomic(n->u.repeat.e)) {
			pp_iter(f, opt, n->u.repeat.e);
		} else {
			fprintf(f, "(?:");
			pp_iter(f, opt, n->u.repeat.e);
			fprintf(f, ")");
		}

		assert(n->u.repeat.max == AST_COUNT_UNBOUNDED || n->u.repeat.max >= n->u.repeat.min);

		for (i = 0; i < sizeof a / sizeof *a; i++) {
			if (a[i].m == n->u.repeat.min && a[i].n == n->u.repeat.max) {
				assert(a[i].op != NULL);
				fprintf(f, "%s", a[i].op);
				break;
			}
		}

		if (i == sizeof a / sizeof *a) {
			fprintf(f, "{");
			fprintf(f, "%u", n->u.repeat.min);
			if (n->u.repeat.max == n->u.repeat.min) {
				/* nothing */
			} else if (n->u.repeat.max == AST_COUNT_UNBOUNDED) {
				fprintf(f, ",");
			} else {
				fprintf(f, ",");
				fprintf(f, "%u", n->u.repeat.max);
			}
			fprintf(f, "}");
		}

		break;
	}

	case AST_EXPR_GROUP:
		fprintf(f, "(");
		pp_iter(f, opt, n->u.group.e);
		fprintf(f, ")");
		break;

	case AST_EXPR_ANCHOR:
		assert(n->u.anchor.type == AST_ANCHOR_START || n->u.anchor.type == AST_ANCHOR_END);
		fprintf(f, "%s", n->u.anchor.type == AST_ANCHOR_START ? "^" : "$");
		break;

	case AST_EXPR_SUBTRACT:
		assert(!"unimplemented");
		pp_iter(f, opt, n->u.subtract.a);
		fprintf(f, "-");
		pp_iter(f, opt, n->u.subtract.b);
		break;

	case AST_EXPR_RANGE:
		fprintf(f, "[");
		print_endpoint(f, opt, &n->u.range.from);
		fprintf(f, "-");
		print_endpoint(f, opt, &n->u.range.to);
		fprintf(f, "]");
		break;

	case AST_EXPR_FLAGS:
		fprintf(f, "\tn%p [ label = <{FLAGS|{+", (void *) n);
		re_flags_print(f, n->u.flags.pos);
		fprintf(f, "|-");
		re_flags_print(f, n->u.flags.neg);
		fprintf(f, "> ];\n");
		break;

	case AST_EXPR_TOMBSTONE:
		fprintf(f, "[^\\x00-\\xff]");
		break;

	default:
		assert(!"unreached");
	}
}

void
ast_print_pcre(FILE *f, const struct fsm_options *opt,
	const struct ast *ast)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(ast != NULL);

	pp_iter(f, opt, ast->expr);

	fprintf(f, "\n");
}

