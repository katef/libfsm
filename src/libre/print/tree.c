/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>

#include <re/re.h>

#include "../class.h"
#include "../ast.h"
#include "../print.h"

#define INDENT(F, IND)                                 \
    do {                                               \
        size_t i;                                      \
        for (i = 0; i < IND; i++) { fprintf(F, " "); } \
    } while (0)                                        \

static void
print_char_or_esc(FILE *f, unsigned char c)
{
	if (isprint(c)) {
		fprintf(f, "%c", c);
	} else {
		fprintf(f, "\\x%02x", c);
	}
}

static void
fprintf_count(FILE *f, unsigned count)
{
	if (count == AST_COUNT_UNBOUNDED) {
		fprintf(f, "inf");
	} else {
		fprintf(f, "%u", count);
	}
}

static void
fprintf_flags(FILE *f, enum ast_flags flags)
{
	fprintf(f, "(f: ");

#define PR_FLAG(X, S)                           \
    if (0 != (flags & (AST_FLAG_ ## X))) { \
        fprintf(f, "%s", S);                    \
    }
    
	PR_FLAG(FIRST,         "F");
	PR_FLAG(LAST,          "L");
	PR_FLAG(UNSATISFIABLE, "X");
	PR_FLAG(NULLABLE,      "0");

#undef PR_FLAG

	fprintf(f, ")");
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
print_endpoint(FILE *f, const struct ast_endpoint *e)
{
	switch (e->type) {
	case AST_ENDPOINT_LITERAL:
		fprintf(f, "'");
		print_char_or_esc(f, e->u.literal.c);
		fprintf(f, "'");
		break;

	case AST_ENDPOINT_CODEPOINT:
		fprintf(f, "U+%lX\n", (unsigned long) e->u.codepoint.u);
		break;

	default:
		assert(!"unreached");
	}
}

static void
pp_iter(FILE *f, const struct fsm_options *opt, size_t indent, struct ast_expr *n)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (n == NULL) { return; }

	INDENT(f, indent);

#define IND 4

	fprintf(f, "%p", (void *) n);
	if (n->flags != 0x00) {
		fprintf(f, " ");
		fprintf_flags(f, n->flags);
	}		
	fprintf(f, " -- ");

	switch (n->type) {
	case AST_EXPR_EMPTY:
		fprintf(f, "EMPTY \n");
		break;

	case AST_EXPR_CONCAT: {
		size_t i, count = n->u.concat.count;
		fprintf(f, "CONCAT (%u):\n", (unsigned)count);

		for (i = 0; i < count; i++) {
			pp_iter(f, opt, indent + 1 * IND, n->u.concat.n[i]);
		}
		break;
	}

	case AST_EXPR_ALT: {
		size_t i, count = n->u.alt.count;
		fprintf(f, "ALT (%u):\n", (unsigned)count);
		for (i = 0; i < count; i++) {
			pp_iter(f, opt, indent + 1 * IND, n->u.alt.n[i]);
		}
		break;
	}

	case AST_EXPR_LITERAL:
		fprintf(f, "LITERAL '%c'\n", n->u.literal.c);
		break;

	case AST_EXPR_CODEPOINT:
		fprintf(f, "CODEPOINT U+%lX\n", (unsigned long) n->u.codepoint.u);
		break;

	case AST_EXPR_ANY:
		fprintf(f, "ANY:\n");
		break;

	case AST_EXPR_REPEAT:
		fprintf(f, "REPEAT {");
		fprintf_count(f, n->u.repeat.min);
		fprintf(f, ",");
		fprintf_count(f, n->u.repeat.max);
		fprintf(f, "}\n");
		pp_iter(f, opt, indent + 1 * IND, n->u.repeat.e);
		break;

	case AST_EXPR_GROUP:
		fprintf(f, "GROUP %p: %u\n", (void *) n, n->u.group.id);
		pp_iter(f, opt, indent + 1 * IND, n->u.group.e);
		break;

	case AST_EXPR_FLAGS:
		fprintf(f, "FLAGS pos:(");
		re_flags_print(f, n->u.flags.pos);
		fprintf(f, "), neg:(");
		re_flags_print(f, n->u.flags.neg);
		fprintf(f, ")\n");
		break;

	case AST_EXPR_ANCHOR:
		assert(n->u.anchor.type == AST_ANCHOR_START || n->u.anchor.type == AST_ANCHOR_END);
		fprintf(f, "ANCHOR %s\n", n->u.anchor.type == AST_ANCHOR_START ? "^" : "$");
		break;

	case AST_EXPR_SUBTRACT:
		fprintf(f, "SUBTRACT %p:\n", (void *) n);
		pp_iter(f, opt, indent + 4, n->u.subtract.a);
		pp_iter(f, opt, indent + 4, n->u.subtract.b);
		break;

	case AST_EXPR_RANGE:
		fprintf(f, "RANGE %p: ", (void *) n);
		print_endpoint(f, &n->u.range.from);
		fprintf(f, "-");
		print_endpoint(f, &n->u.range.to);
		fprintf(f, "\n");
		break;

	case AST_EXPR_TOMBSTONE:
		fprintf(f, "RIP\n");
		break;

	default:
		assert(!"unreached");
	}
}

void
ast_print_tree(FILE *f, const struct fsm_options *opt,
	const struct ast *ast)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(ast != NULL);

	pp_iter(f, opt, 0, ast->expr);
}

