/*
 * Copyright 2018 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <inttypes.h>

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
	const char *sep = "";

	if (fl & RE_ICASE  ) { fprintf(f, "%sicase",    sep); sep = " "; }
	if (fl & RE_TEXT   ) { fprintf(f, "%stext ",    sep); sep = " "; }
	if (fl & RE_MULTI  ) { fprintf(f, "%smulti ",   sep); sep = " "; }
	if (fl & RE_REVERSE) { fprintf(f, "%sreverse ", sep); sep = " "; }
	if (fl & RE_SINGLE ) { fprintf(f, "%ssingle ",  sep); sep = " "; }
	if (fl & RE_ZONE   ) { fprintf(f, "%szone ",    sep); sep = " "; }
}

static void
print_endpoint(FILE *f, const struct fsm_options *opt, const struct ast_endpoint *e)
{
	switch (e->type) {
	case AST_ENDPOINT_LITERAL:
		dot_escputc_html_record(f, opt, (char)e->u.literal.c);
		break;

	case AST_ENDPOINT_CODEPOINT:
		fprintf(f, "U+%" PRIX32, e->u.codepoint.u);
		break; 

	default:
		assert(!"unreached");
		break;
	}
}

static void
pp_iter(FILE *f, const struct fsm_options *opt,
	const void *parent, enum re_flags re_flags, struct ast_expr *n)
{
	const char *typestr;

	assert(f != NULL);
	assert(opt != NULL);

	if (n == NULL) { return; }

	if (parent != NULL) {
		fprintf(f, "\tn%p -> n%p;\n", parent, (void *) n);
	}

	switch (n->type) {
	case AST_EXPR_EMPTY:
		typestr = "EMPTY";
		break;

	case AST_EXPR_CONCAT:
		typestr = "CONCAT";
		break;

	case AST_EXPR_ALT:
		typestr = "ALT";
		break;

	case AST_EXPR_LITERAL:
		typestr = "LITERAL";
		break;

	case AST_EXPR_CODEPOINT:
		typestr = "CODEPOINT";
		break;

	case AST_EXPR_REPEAT:
		typestr = "REPEAT";
		break;

	case AST_EXPR_GROUP:
		typestr = "GROUP";
		break;

	case AST_EXPR_ANCHOR:
		typestr = "ANCHOR";
		break;

	case AST_EXPR_SUBTRACT:
		typestr = "SUBTRACT";
		break;

	case AST_EXPR_RANGE:
		typestr = "RANGE";
		break;

	case AST_EXPR_TOMBSTONE:
		typestr = "RIP";
		break;

	default:
		assert(!"unreached");
	}

	fprintf(f, "\tn%p [ label = <{%s", (void *) n, typestr);

	if (n->re_flags != re_flags) {
		fprintf(f, "|flags:");
		if (n->re_flags & ~re_flags) {
			fprintf(f, " pos(");
			re_flags_print(f, n->re_flags & ~re_flags);
			fprintf(f, ")");
		}
		if (re_flags & ~n->re_flags) {
			fprintf(f, " neg(");
			re_flags_print(f, re_flags & ~n->re_flags);
			fprintf(f, ")");
		}
		re_flags = n->re_flags;
	}

	switch (n->type) {
	default:
		break;

	case AST_EXPR_CONCAT:
		fprintf(f, "}|{%zu", n->u.concat.count);
		break;

	case AST_EXPR_ALT:
		fprintf(f, "}|{%zu", n->u.alt.count);
		break;

	case AST_EXPR_LITERAL:
		fprintf(f, "}|{");
		dot_escputc_html_record(f, opt, n->u.literal.c);
		break;

	case AST_EXPR_CODEPOINT:
		fprintf(f, "}|{U+%" PRIX32, n->u.codepoint.u);
		break;

	case AST_EXPR_REPEAT:
		fprintf(f, "}|{&#x7b;");
		fprintf_count(f, n->u.repeat.min);
		fprintf(f, ", ");
		fprintf_count(f, n->u.repeat.max);
		fprintf(f, "&#x7d;");
		break;

	case AST_EXPR_GROUP:
		fprintf(f, "}|{#%u", n->u.group.id);
		break;

	case AST_EXPR_ANCHOR:
		assert(n->u.anchor.type == AST_ANCHOR_START || n->u.anchor.type == AST_ANCHOR_END);
		fprintf(f, "}|{%c", n->u.anchor.type == AST_ANCHOR_START ? '^' : '$');
		break;

	case AST_EXPR_SUBTRACT:
		fprintf(f, "}|{a|b");
		break;

	case AST_EXPR_RANGE:
		fprintf(f, "}|{");
		print_endpoint(f, opt, &n->u.range.from);
		fprintf(f, " &ndash; ");
		print_endpoint(f, opt, &n->u.range.to);
		break;
	}

	fprintf(f, "}> ];\n");

	switch (n->type) {
	default:
		break;

	case AST_EXPR_CONCAT:
	{
		size_t i;
		for (i = 0; i < n->u.concat.count; i++) {
			pp_iter(f, opt, n, re_flags, n->u.concat.n[i]);
		}
		break;
	}

	case AST_EXPR_ALT:
	{
		size_t i;
		for (i = 0; i < n->u.alt.count; i++) {
			pp_iter(f, opt, n, re_flags, n->u.alt.n[i]);
		}
		break;
	}

	case AST_EXPR_REPEAT:
		pp_iter(f, opt, n, re_flags, n->u.repeat.e);
		break;

	case AST_EXPR_GROUP:
		pp_iter(f, opt, n, re_flags, n->u.group.e);
		break;

	case AST_EXPR_SUBTRACT:
		pp_iter(f, opt, n, re_flags, n->u.subtract.a);
		pp_iter(f, opt, n, re_flags, n->u.subtract.b);
		break;
	}
}

void
ast_print_dot(FILE *f, const struct fsm_options *opt,
	enum re_flags re_flags, const struct ast *ast)
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

	pp_iter(f, opt, NULL, re_flags, ast->expr);

	fprintf(f, "}\n");
}

