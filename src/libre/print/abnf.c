/*
 * Copyright 2018 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <fsm/fsm.h>

#include <re/re.h>

#include <print/esc.h>

#include "../class.h"
#include "../ast.h"
#include "../print.h"

static void
pp_iter(FILE *f, const struct fsm_options *opt, struct ast_expr *n);

static int
atomic(struct ast_expr *n)
{
	switch (n->type) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:
	case AST_EXPR_ANY:
	case AST_EXPR_GROUP:
	case AST_EXPR_RANGE:
	case AST_EXPR_TOMBSTONE:
		return 1;

	case AST_EXPR_REPEAT:
		return 0;

	case AST_EXPR_FLAGS:
		return 0; /* XXX */

	default:
		assert(!"unreached");
		abort();
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
pp_iter(FILE *f, const struct fsm_options *opt, struct ast_expr *n)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (n == NULL) {
		return;
	}

	switch (n->type) {
	case AST_EXPR_EMPTY:
		break;

	case AST_EXPR_CONCAT: {
		size_t i;

		for (i = 0; i < n->u.concat.count; i++) {
			pp_iter(f, opt, n->u.concat.n[i]);
			if (i + 1 < n->u.concat.count) {
				fprintf(f, " ");
			}
		}
		break;
	}

	case AST_EXPR_ALT: {
		size_t i;

		for (i = 0; i < n->u.alt.count; i++) {
			pp_iter(f, opt, n->u.alt.n[i]);
			if (i + 1 < n->u.alt.count) {
				fprintf(f, " / "); /* XXX: indent */
			}
		}
		break;
	}

	case AST_EXPR_LITERAL:
		abnf_escputc(f, opt, n->u.literal.c);
		break;

	case AST_EXPR_CODEPOINT:
		assert(!"unimplemented");
		fprintf(f, "%%x\"%lX\"", (unsigned long) n->u.codepoint.u);
		break;

	case AST_EXPR_ANY:
		fprintf(f, "OCTET"); /* ABNF core rule for %x00-FF */
		break;

	case AST_EXPR_REPEAT: {
		assert(n->u.repeat.min != AST_COUNT_UNBOUNDED);

		if (n->u.repeat.min == 0 && n->u.repeat.max == 1) {
			fprintf(f, "[ ");
			pp_iter(f, opt, n->u.repeat.e);
			fprintf(f, " ]");
			return;
		}

		if (n->u.repeat.min == 1 && n->u.repeat.max == 1) {
			pp_iter(f, opt, n->u.repeat.e);
			return;
		}

		if (n->u.repeat.min == n->u.repeat.max) {
			fprintf(f, "%u", n->u.repeat.max);
			print_grouped(f, opt, n->u.repeat.e);
			return;
		}

		if (n->u.repeat.min > 0) {
			fprintf(f, "%u", n->u.repeat.min);
		}

		fprintf(f, "*");

		if (n->u.repeat.max != AST_COUNT_UNBOUNDED) {
			fprintf(f, "%u", n->u.repeat.max);
		}

		print_grouped(f, opt, n->u.repeat.e);

		break;
	}

	case AST_EXPR_GROUP:
		fprintf(f, "(");
		pp_iter(f, opt, n->u.group.e);
		fprintf(f, ")");
		break;

	case AST_EXPR_ANCHOR:
		assert(n->u.anchor.type == AST_ANCHOR_START
		    || n->u.anchor.type == AST_ANCHOR_END);
		fprintf(f, "%s", n->u.anchor.type == AST_ANCHOR_START ? "<^>" : "<$>");
		break;

	case AST_EXPR_SUBTRACT:
		assert(!"unimplemented");
		pp_iter(f, opt, n->u.subtract.a);
		fprintf(f, " - ");
		pp_iter(f, opt, n->u.subtract.b);
		break;

	case AST_EXPR_RANGE: {
		if (n->u.range.from.type != AST_ENDPOINT_LITERAL
		 || n->u.range.to.type != AST_ENDPOINT_LITERAL)
		{
			assert(!"unimplemented");
			abort();
		}

		fprintf(f, "%%x%02X-%02X",
			(unsigned char) n->u.range.from.u.literal.c,
			(unsigned char) n->u.range.to.u.literal.c);
		}
		break;

	case AST_EXPR_FLAGS:
		abort();

	case AST_EXPR_TOMBSTONE:
		fprintf(f, "<RIP>");
		break;

	default:
		assert(!"unreached");
	}
}

void
ast_print_abnf(FILE *f, const struct fsm_options *opt,
	const struct ast *ast)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(ast != NULL);

	fprintf(f, "e = ");

	pp_iter(f, opt, ast->expr);

	fprintf(f, "\n");
	fprintf(f, "\n");
}

