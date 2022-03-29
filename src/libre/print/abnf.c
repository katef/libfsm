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
#include <inttypes.h>

#include <re/re.h>

#include <print/esc.h>

#include "../class.h"
#include "../ast.h"
#include "../print.h"

static void
pp_iter(FILE *f, const struct fsm_options *opt, enum re_flags *re_flags, struct ast_expr *n);

static int
prec(const struct ast_expr *n)
{
	switch (n->type) {
	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:
	case AST_EXPR_RANGE:
	case AST_EXPR_SUBTRACT:
	case AST_EXPR_GROUP:
	case AST_EXPR_TOMBSTONE:
		return 1;

	case AST_EXPR_REPEAT:
		return 1;

	case AST_EXPR_ANCHOR:
	case AST_EXPR_EMPTY:
		/* anchor and empty are special: although they take no
		 * operands, it is not possible to get these as the
		 * operand of a repeat expression without parentheses:
		 * "*" or "^*" are syntax errors, but "(?:)*" and
		 * "(?:^)*" are valid */
		return 2;

	case AST_EXPR_CONCAT:
		return 3;

	case AST_EXPR_ALT:
		return 4;

	default:
		assert(!"unreached");
		abort();
	}
}

static void
pp_atomic(FILE *f, const struct fsm_options *opt, enum re_flags *re_flags,
	struct ast_expr *n, const struct ast_expr *parent)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (prec(n) <= prec(parent)) {
		pp_iter(f, opt, re_flags, n);
	} else {
		enum re_flags local_flags = *re_flags;
		fprintf(f, "(");
		pp_iter(f, opt, &local_flags, n);
		fprintf(f, ")");
	}
}

static void
pp_iter(FILE *f, const struct fsm_options *opt, enum re_flags *re_flags, struct ast_expr *n)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (n == NULL) {
		return;
	}

	if ((n->re_flags & AST_FLAG_UNSATISFIABLE) != 0) {
		fprintf(f, "<RIP>");
		return;
	}

	/* other values of flags are informative only,
	 * and we don't output them here */
	(void) n->re_flags;

	switch (n->type) {
	case AST_EXPR_EMPTY:
		break;

	case AST_EXPR_CONCAT: {
		size_t i;

		for (i = 0; i < n->u.concat.count; i++) {
			pp_atomic(f, opt, re_flags, n->u.concat.n[i], n);
			if (i + 1 < n->u.concat.count) {
				fprintf(f, " ");
			}
		}

		break;
	}

	case AST_EXPR_ALT: {
		size_t i;
		unsigned empty_count;

		/*
		 * ABNF seems to provide no way to express an empty node;
		 * there's no core rule for it, and the grammar does not allow
		 * for () or an empty production.
		 *
		 * So here we have special handling; if we have empty alts,
		 * the entire list is wrapped with [...] and we skip the nodes
		 * when outputting the list.
		 */

		empty_count = 0;
		for (i = 0; i < n->u.alt.count; i++) {
			if (n->u.alt.n[i]->type == AST_EXPR_EMPTY) {
				empty_count++;
			}
		}

		if (empty_count > 0) {
			fprintf(f, "[ ");
		}

		for (i = 0; i < n->u.alt.count; i++) {
			if (n->u.alt.n[i]->type != AST_EXPR_EMPTY) {
				pp_atomic(f, opt, re_flags, n->u.alt.n[i], n);
			}

			if (i + 1 < n->u.alt.count - empty_count) {
				fprintf(f, " / "); /* XXX: indent */
			}
		}
		
		if (empty_count > 0) {
			fprintf(f, " ]");
		}

		break;
	}

	case AST_EXPR_LITERAL:
		abnf_escputc(f, opt, n->u.literal.c);
		break;

	case AST_EXPR_CODEPOINT:
		assert(!"unimplemented");
		fprintf(f, "%%x\"%" PRIX32 "\"", n->u.codepoint.u);
		break;

	case AST_EXPR_REPEAT: {
		assert(n->u.repeat.min != AST_COUNT_UNBOUNDED);

		if (n->u.repeat.min == 0 && n->u.repeat.max == 1) {
			fprintf(f, "[ ");
			pp_iter(f, opt, re_flags, n->u.repeat.e);
			fprintf(f, " ]");
			return;
		}

		if (n->u.repeat.min == 1 && n->u.repeat.max == 1) {
			pp_atomic(f, opt, re_flags, n->u.repeat.e, n);
			return;
		}

		if (n->u.repeat.min == n->u.repeat.max) {
			fprintf(f, "%u", n->u.repeat.max);
			pp_atomic(f, opt, re_flags, n->u.repeat.e, n);
			return;
		}

		if (n->u.repeat.min > 0) {
			fprintf(f, "%u", n->u.repeat.min);
		}

		fprintf(f, "*");

		if (n->u.repeat.max != AST_COUNT_UNBOUNDED) {
			fprintf(f, "%u", n->u.repeat.max);
		}

		pp_atomic(f, opt, re_flags, n->u.repeat.e, n);

		break;
	}

	case AST_EXPR_GROUP:
		fprintf(f, "(");
		pp_iter(f, opt, re_flags, n->u.group.e);
		fprintf(f, ")");
		break;

	case AST_EXPR_ANCHOR:
		assert(n->u.anchor.type == AST_ANCHOR_START
		    || n->u.anchor.type == AST_ANCHOR_END);
		fprintf(f, "%s", n->u.anchor.type == AST_ANCHOR_START ? "<^>" : "<$>");
		break;

	case AST_EXPR_SUBTRACT:
		assert(!"unimplemented");
		pp_atomic(f, opt, re_flags, n->u.subtract.a, n);
		fprintf(f, " - ");
		pp_atomic(f, opt, re_flags, n->u.subtract.b, n);
		break;

	case AST_EXPR_RANGE: {
		if (n->u.range.from.type != AST_ENDPOINT_LITERAL
		 || n->u.range.to.type != AST_ENDPOINT_LITERAL)
		{
			assert(!"unimplemented");
			abort();
		}

		if (n->u.range.from.u.literal.c == 0x00 &&
			n->u.range.to.u.literal.c == 0xff)
		{
			fprintf(f, "OCTET"); /* ABNF core rule for %x00-FF */
			break;
		}

		fprintf(f, "%%x%02X-%02X",
			(unsigned char) n->u.range.from.u.literal.c,
			(unsigned char) n->u.range.to.u.literal.c);
		}
		break;

	case AST_EXPR_TOMBSTONE:
		fprintf(f, "<RIP>");
		break;

	default:
		assert(!"unreached");
	}
}

void
ast_print_abnf(FILE *f, const struct fsm_options *opt,
	enum re_flags re_flags, const struct ast *ast)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(ast != NULL);

	(void) re_flags;

	fprintf(f, "e = ");

	pp_iter(f, opt, &re_flags, ast->expr);

	fprintf(f, "\n");
	fprintf(f, "\n");
}

