/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
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

#define PR_FLAG(X, S)                         \
    if (0 != (flags & (RE_AST_FLAG_ ## X))) { \
        fprintf(f, "%s", S);                  \
    }
    
	PR_FLAG(FIRST_STATE,   "F");
	PR_FLAG(LAST_STATE,    "L");
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
print_range_endpoint(FILE *f, const struct ast_range_endpoint *r)
{
	switch (r->type) {
	case AST_RANGE_ENDPOINT_LITERAL:
		fprintf(f, "'");
		print_char_or_esc(f, r->u.literal.c);
		fprintf(f, "'");
		break;

	default:
		assert(!"unreached");
	}
}

static void
cc_pp_iter(FILE *f, const struct fsm_options *opt,
	struct ast_class *n, size_t indent)
{
	assert(f != NULL);
	assert(opt != NULL);
	assert(n != NULL);

	INDENT(f, indent);

	switch (n->type) {
	case AST_CLASS_CONCAT:
		fprintf(f, "CLASS-CONCAT %p: \n", (void *) n);
		cc_pp_iter(f, opt, n->u.concat.l, indent + 4);
		cc_pp_iter(f, opt, n->u.concat.r, indent + 4);
		break;

	case AST_CLASS_LITERAL:
		fprintf(f, "CLASS-LITERAL %p: '", (void *) n);
		print_char_or_esc(f, n->u.literal.c);
		fprintf(f, "'\n");
		break;

	case AST_CLASS_RANGE:
		fprintf(f, "CLASS-RANGE %p: ", (void *) n);
		print_range_endpoint(f, &n->u.range.from);
		fprintf(f, "-");
		print_range_endpoint(f, &n->u.range.to);
		fprintf(f, "\n");
		break;

	case AST_CLASS_NAMED:
		fprintf(f, "CLASS-NAMED %p: %s\n",
		    (void *) n, class_name(n->u.named.ctor));
		break;

	case AST_CLASS_FLAGS:
		fprintf(f, "CLASS-FLAGS %p: [", (void *) n);
		if (n->u.flags.f & AST_CLASS_FLAG_INVERTED) { 
			fprintf(f, "^");
		}
		if (n->u.flags.f & AST_CLASS_FLAG_MINUS) {
			fprintf(f, "-");
		}
		fprintf(f, "]\n");
		break;

	case AST_CLASS_SUBTRACT:
		fprintf(f, "CLASS-SUBTRACT %p:\n", (void *) n);
		cc_pp_iter(f, opt, n->u.subtract.ast, indent + 4);
		cc_pp_iter(f, opt, n->u.subtract.mask, indent + 4);
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

	/*
	 * CONCAT and ALT should be removed once CONCAT_N and ALT_N are
	 * built directly in the parser.
	 */
	case AST_EXPR_CONCAT:
		fprintf(f, "CONCAT %p: {\n", (void *) n);
		pp_iter(f, opt, indent + 1 * IND, n->u.concat.l);
		INDENT(f, indent);

		fprintf(f, ", (%p)\n", (void *) n);

		pp_iter(f, opt, indent + 1 * IND, n->u.concat.r);
		INDENT(f, indent);
		fprintf(f, "} (%p)\n", (void *) n);
		break;

	case AST_EXPR_ALT:
		fprintf(f, "ALT %p: {\n", (void *) n);
		pp_iter(f, opt, indent + 1 * IND, n->u.alt.l);
		INDENT(f, indent);

		fprintf(f, ", (%p)\n", (void *) n);
		pp_iter(f, opt, indent + 1 * IND, n->u.alt.r);
		INDENT(f, indent);
		fprintf(f, "} (%p)\n", (void *) n);
		break;

	case AST_EXPR_CONCAT_N: {
		size_t i, count = n->u.concat_n.count;
		fprintf(f, "CONCAT (%u):\n", (unsigned)count);

		for (i = 0; i < count; i++) {
			pp_iter(f, opt, indent + 1 * IND, n->u.concat_n.n[i]);
		}
		break;
	}

	case AST_EXPR_ALT_N: {
		size_t i, count = n->u.alt_n.count;
		fprintf(f, "ALT (%u):\n", (unsigned)count);
		for (i = 0; i < count; i++) {
			pp_iter(f, opt, indent + 1 * IND, n->u.alt_n.n[i]);
		}
		break;
	}

	case AST_EXPR_LITERAL:
		fprintf(f, "LITERAL '%c'\n", n->u.literal.c);
		break;

	case AST_EXPR_ANY:
		fprintf(f, "ANY:\n");
		break;

	case AST_EXPR_REPEATED:
		fprintf(f, "REPEATED {");
		fprintf_count(f, n->u.repeated.low);
		fprintf(f, ",");
		fprintf_count(f, n->u.repeated.high);
		fprintf(f, "}\n");
		pp_iter(f, opt, indent + 1 * IND, n->u.repeated.e);
		break;

	case AST_EXPR_CLASS:
		fprintf(f, "CHAR_CLASS %p: \n", (void *) n);
		cc_pp_iter(f, opt, n->u.class.class, indent + IND);
		fprintf(f, "\n");
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
		fprintf(f, "ANCHOR %s\n",
		    n->u.anchor.type == RE_AST_ANCHOR_START ? "^"
		    : n->u.anchor.type == RE_AST_ANCHOR_END ? "$"
		    : "<matchfail>");
		break;

	case AST_EXPR_TOMBSTONE:
		fprintf(f, "<tombstone>\n");
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

