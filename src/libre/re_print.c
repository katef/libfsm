#include "re_print.h"
#include "re_char_class.h"

static void
re_flags_print(FILE *f, enum re_flags fl);

static void
cc_pp_iter(FILE *f, struct re_char_class_ast *n, size_t indent);

static void
print_char_or_esc(FILE *f, unsigned char c);

static void
print_range_endpoint(FILE *f, const struct ast_range_endpoint *r);


#define INDENT(F, IND)							\
	do {								\
		size_t i;						\
		for (i = 0; i < IND; i++) { fprintf(F, " "); }		\
	} while(0) 							\

static void
pp_iter(FILE *f, size_t indent, struct ast_expr *n)
{
	if (n == NULL) { return; }
	/* assert(n != NULL); */
	INDENT(f, indent);

	#define IND 4

	switch (n->t) {
	case AST_EXPR_EMPTY:
		fprintf(f, "EMPTY %p\n", (void *)n);
		break;
	case AST_EXPR_CONCAT:
		fprintf(f, "CONCAT %p: {\n", (void *)n);
		pp_iter(f, indent + 1*IND, n->u.concat.l);
		INDENT(f, indent);
		fprintf(f, ", (%p)\n", (void *)n);
		pp_iter(f, indent + 1*IND, n->u.concat.r);
		INDENT(f, indent);
		fprintf(f, "} (%p)\n", (void *)n);
		break;
	case AST_EXPR_ALT:
		fprintf(f, "ALT %p: {\n", (void *)n);
		pp_iter(f, indent + 1*IND, n->u.alt.l);
		INDENT(f, indent);
		fprintf(f, ", (%p)\n", (void *)n);
		pp_iter(f, indent + 1*IND, n->u.alt.r);
		INDENT(f, indent);
		fprintf(f, "} (%p)\n", (void *)n);
		break;
	case AST_EXPR_LITERAL:
		fprintf(f, "LITERAL %p: '%c'\n", (void *)n, n->u.literal.c);
		break;
	case AST_EXPR_ANY:
		fprintf(f, "ANY %p:\n", (void *)n);
		break;
	case AST_EXPR_MANY:
		fprintf(f, "MANY %p:\n", (void *)n);
		break;
	case AST_EXPR_KLEENE:
		fprintf(f, "KLEENE %p\n", (void *)n);
		pp_iter(f, indent + 1*IND, n->u.kleene.e);
		break;
	case AST_EXPR_PLUS:
		fprintf(f, "PLUS %p\n", (void *)n);
		pp_iter(f, indent + 1*IND, n->u.plus.e);
		break;
	case AST_EXPR_OPT:
		fprintf(f, "OPT %p\n", (void *)n);
		pp_iter(f, indent + 1*IND, n->u.opt.e);
		break;
	case AST_EXPR_REPEATED:
		fprintf(f, "REPEATED %p: (%u,%u)\n",
		    (void *)n, n->u.repeated.low, n->u.repeated.high);
		pp_iter(f, indent + 1*IND, n->u.repeated.e);
		break;
	case AST_EXPR_CHAR_CLASS:
		fprintf(f, "CHAR_CLASS %p: \n", (void *)n);
		cc_pp_iter(f, n->u.char_class.cca, indent + IND);
		fprintf(f, "\n");
		break;
	case AST_EXPR_CHAR_TYPE:
		fprintf(f, "CHAR_TYPE %p: %d\n", (void *)n, n->u.char_type.id);
		break;
	case AST_EXPR_GROUP:
		fprintf(f, "GROUP %p: %u\n", (void *)n, n->u.group.id);
		pp_iter(f, indent + 1*IND, n->u.group.e);
		break;
	case AST_EXPR_FLAGS:
		fprintf(f, "FLAGS %p: pos:(", (void *)n);
		re_flags_print(f, n->u.flags.pos);
		fprintf(f, "), neg:(");
		re_flags_print(f, n->u.flags.neg);
		fprintf(f, ")\n");
		break;
	default:
		assert(0);
	}
}

void
re_ast_print(FILE *f, struct ast_re *ast)
{
	pp_iter(f, 0, ast->expr);
}

static void
re_flags_print(FILE *f, enum re_flags fl)
{
	const char *sep = "";
	if (fl & RE_ICASE) { fprintf(f, "%sicase", sep); sep = " "; }
	if (fl & RE_TEXT) { fprintf(f, "%stext ", sep); sep = " "; }
	if (fl & RE_MULTI) { fprintf(f, "%smulti ", sep); sep = " "; }
	if (fl & RE_REVERSE) { fprintf(f, "%sreverse ", sep); sep = " "; }
	if (fl & RE_SINGLE) { fprintf(f, "%ssingle ", sep); sep = " "; }
	if (fl & RE_ZONE) { fprintf(f, "%szone ", sep); sep = " "; }
}

static void
print_range_endpoint(FILE *f, const struct ast_range_endpoint *r)
{
	switch (r->t) {
	case AST_RANGE_ENDPOINT_LITERAL:
		fprintf(f, "'");
		print_char_or_esc(f, r->u.literal.c);
		fprintf(f, "'");
		break;
	case AST_RANGE_ENDPOINT_CHAR_TYPE:
		fprintf(f, "%s", re_char_class_type_id_str(r->u.char_type.id));
		break;
	case AST_RANGE_ENDPOINT_CHAR_CLASS:
		fprintf(f, "[:%s:]", re_char_class_id_str(r->u.char_class.id));
		break;
	}
}

static void
cc_pp_iter(FILE *f, struct re_char_class_ast *n, size_t indent)
{
	size_t i;
	assert(n != NULL);
	for (i = 0; i < indent; i++) { fprintf(f, " "); }

	switch (n->t) {
	case RE_CHAR_CLASS_AST_CONCAT:
		fprintf(f, "CLASS-CONCAT %p: \n", (void *)n);
		cc_pp_iter(f, n->u.concat.l, indent + 4);
		cc_pp_iter(f, n->u.concat.r, indent + 4);
		break;
	case RE_CHAR_CLASS_AST_LITERAL:
		fprintf(f, "CLASS-LITERAL %p: '", (void *)n);
		print_char_or_esc(f, n->u.literal.c);
		fprintf(f, "'\n");
		break;
	case RE_CHAR_CLASS_AST_RANGE:
		fprintf(f, "CLASS-RANGE %p: ", (void *)n);
		print_range_endpoint(f, &n->u.range.from);
		fprintf(f, "-");
		print_range_endpoint(f, &n->u.range.to);
		fprintf(f, "\n");
		break;
	case RE_CHAR_CLASS_AST_NAMED:
		fprintf(f, "CLASS-NAMED %p: '%s'\n",
		    (void *)n, re_char_class_id_str(n->u.named.id));
		break;
	case RE_CHAR_CLASS_AST_CHAR_TYPE:
		fprintf(f, "CLASS-CHAR-TYPE %p: %s\n",
		    (void *)n, re_char_class_type_id_str(n->u.char_type.id));
		break;
	case RE_CHAR_CLASS_AST_FLAGS:
		fprintf(f, "CLASS-FLAGS %p: [", (void *)n);
		if (n->u.flags.f & RE_CHAR_CLASS_FLAG_INVERTED) { 
			fprintf(f, "^");
		}
		if (n->u.flags.f & RE_CHAR_CLASS_FLAG_MINUS) {
			fprintf(f, "-");
		}
		fprintf(f, "]\n");
		break;
	case RE_CHAR_CLASS_AST_SUBTRACT:
		fprintf(f, "CLASS-SUBTRACT %p:\n", (void *)n);
		cc_pp_iter(f, n->u.subtract.ast, indent + 4);
		cc_pp_iter(f, n->u.subtract.mask, indent + 4);
		break;
	default:
		fprintf(stderr, "(MATCH FAIL)\n");
		assert(0);
	}
}

static void
print_char_or_esc(FILE *f, unsigned char c)
{
	if (isprint(c)) {
		fprintf(f, "%c", c);
	} else {
		fprintf(f, "\\x%02x", c);
	}
}
