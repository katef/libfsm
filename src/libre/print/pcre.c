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

#include <re/re.h>

#include <print/esc.h>

#include "../class.h"
#include "../class_lookup.h"
#include "../ast.h"
#include "../print.h"

static int
prec(struct ast_expr *n)
{
	switch (n->type) {
	case AST_EXPR_LITERAL:
	case AST_EXPR_CODEPOINT:
	case AST_EXPR_RANGE:
	case AST_EXPR_SUBTRACT:
	case AST_EXPR_GROUP:
	case AST_EXPR_TOMBSTONE:
		return 0;

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
re_flags_print(FILE *f, enum re_flags fl)
{
	if (fl & RE_ICASE  ) { fprintf(f, "i"); }
	if (fl & RE_TEXT   ) { fprintf(f, "g"); }
	if (fl & RE_MULTI  ) { fprintf(f, "m"); }
	if (fl & RE_REVERSE) { fprintf(f, "r"); }
	if (fl & RE_SINGLE ) { fprintf(f, "s"); }
	if (fl & RE_ZONE   ) { fprintf(f, "z"); }
}

enum {
	RE_FLAGS_PRINTABLE = RE_ICASE | RE_TEXT | RE_MULTI | RE_REVERSE | RE_SINGLE | RE_ZONE
};

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
pp_iter(FILE *f, const struct fsm_options *opt, enum re_flags *re_flags, struct ast_expr *n)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (n == NULL) { return; }

	if ((n->re_flags ^ *re_flags) & RE_FLAGS_PRINTABLE) {
		fprintf(f, "(?");
		re_flags_print(f, n->re_flags & ~*re_flags);
		if (*re_flags & ~n->re_flags & RE_FLAGS_PRINTABLE) {
			fprintf(f, "-");
			re_flags_print(f, *re_flags & ~n->re_flags);
		}
		fprintf(f, ")");
		*re_flags = n->re_flags;
	}

	switch (n->type) {
	case AST_EXPR_EMPTY:
		break;

	case AST_EXPR_CONCAT:
	{
		size_t i;
		for (i = 0; i < n->u.concat.count; i++) {
			if (prec(n->u.concat.n[i]) <= prec(n)) {
				pp_iter(f, opt, re_flags, n->u.concat.n[i]);
			} else {
				enum re_flags localflags = *re_flags;
				fprintf(f, "(?:");
				pp_iter(f, opt, &localflags, n->u.concat.n[i]);
				fprintf(f, ")");
			}
		}
		break;
	}

	case AST_EXPR_ALT:
	{
		size_t i;
		for (i = 0; i < n->u.alt.count; i++) {
			if (prec(n->u.alt.n[i]) <= prec(n)) {
				pp_iter(f, opt, re_flags, n->u.alt.n[i]);
			} else {
				enum re_flags localflags = *re_flags;
				fprintf(f, "(?:");
				pp_iter(f, opt, &localflags, n->u.alt.n[i]);
				fprintf(f, ")");
			}
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

		if (prec(n->u.repeat.e) <= prec(n)) {
			pp_iter(f, opt, re_flags, n->u.repeat.e);
		} else {
			enum re_flags local_flags = *re_flags;
			fprintf(f, "(?:");
			pp_iter(f, opt, &local_flags, n->u.repeat.e);
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

	case AST_EXPR_GROUP: {
		enum re_flags local_flags = *re_flags;
		fprintf(f, "(");
		pp_iter(f, opt, &local_flags, n->u.group.e);
		fprintf(f, ")");
		break;
	}

	case AST_EXPR_ANCHOR:
		assert(n->u.anchor.type == AST_ANCHOR_START || n->u.anchor.type == AST_ANCHOR_END);
		fprintf(f, "%s", n->u.anchor.type == AST_ANCHOR_START ? "^" : "$");
		break;

	case AST_EXPR_SUBTRACT:
		assert(!"unimplemented");
		pp_iter(f, opt, re_flags, n->u.subtract.a);
		fprintf(f, "-");
		pp_iter(f, opt, re_flags, n->u.subtract.b);
		break;

	case AST_EXPR_RANGE:
		if (n->u.range.from.u.literal.c == 0x00 &&
			n->u.range.to.u.literal.c == 0xff)
		{
			fprintf(f, ".");
			break;
		}

		fprintf(f, "[");
		print_endpoint(f, opt, &n->u.range.from);
		fprintf(f, "-");
		print_endpoint(f, opt, &n->u.range.to);
		fprintf(f, "]");
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
	enum re_flags re_flags, const struct ast *ast)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(ast != NULL);

	pp_iter(f, opt, &re_flags, ast->expr);

	fprintf(f, "\n");
}

