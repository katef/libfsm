#include "re_ast.h"

#include <string.h>

static void
re_flags_prettyprint(FILE *f, enum re_flags fl);

struct ast_re *
re_ast_new(void)
{
	struct ast_re *res = calloc(1, sizeof(*res));
	return res;
}

static void
free_iter(struct ast_expr *n)
{
	LOG("%s: %p\n", __func__, (void *)n);
	if (n == NULL) { return; }
	
	switch (n->t) {
	/* These nodes have no subnodes or dynamic allocation */
	case AST_EXPR_EMPTY:
	case AST_EXPR_LITERAL:
	case AST_EXPR_ANY:
	case AST_EXPR_MANY:
	case AST_EXPR_FLAGS:
		break;

	case AST_EXPR_CONCAT:
		free_iter(n->u.concat.l);
		free_iter(n->u.concat.r);
		break;
	case AST_EXPR_ALT:
		free_iter(n->u.alt.l);
		free_iter(n->u.alt.r);
		break;
	case AST_EXPR_KLEENE:
		free_iter(n->u.kleene.e);
		break;
	case AST_EXPR_PLUS:
		free_iter(n->u.plus.e);
		break;
	case AST_EXPR_OPT:
		free_iter(n->u.opt.e);
		break;
	case AST_EXPR_REPEATED:
		free_iter(n->u.repeated.e);
		break;
	case AST_EXPR_CLASS:
		re_char_class_ast_free(n->u.class.cca);
		break;
	case AST_EXPR_GROUP:
		free_iter(n->u.group.e);
		break;
	default:
		assert(0);
	}

	free(n);
}

void
re_ast_free(struct ast_re *ast)
{
	free_iter(ast->expr);
	free(ast);
}

struct ast_expr *
re_ast_expr_empty(void)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_EMPTY;

	LOG("-- %s: %p\n", __func__, (void *)res);
	return res;
}

struct ast_expr *
re_ast_expr_concat(struct ast_expr *l, struct ast_expr *r)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_CONCAT;
	res->u.concat.l = l;
	res->u.concat.r = r;
	LOG("-- %s: %p <- %p, %p\n",
	    __func__, (void *)res, (void *)l, (void *)r);
	assert(l != NULL);
	assert(r != NULL);
	return res;
}

struct ast_expr *
re_ast_expr_alt(struct ast_expr *l, struct ast_expr *r)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_ALT;
	res->u.alt.l = l;
	res->u.alt.r = r;
	LOG("-- %s: %p <- %p, %p\n",
	    __func__, (void *)res, (void *)l, (void *)r);
	assert(l != NULL);
	assert(r != NULL);
	return res;
}

struct ast_expr *
re_ast_expr_literal(char c)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_LITERAL;
	res->u.literal.c = c;
	LOG("-- %s: %p <- %c\n", __func__, (void *)res, c);
	return res;
}

struct ast_expr *
re_ast_expr_any(void)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_ANY;

	LOG("-- %s: %p\n", __func__, (void *)res);
	return res;
}

struct ast_expr *
re_ast_expr_many(void)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_MANY;

	LOG("-- %s: %p\n", __func__, (void *)res);
	return res;
}

struct ast_expr *
re_ast_kleene(struct ast_expr *e)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_KLEENE;
	res->u.kleene.e = e;
	LOG("-- %s: %p <- %p\n", __func__, (void *)res, (void *)e);
	return res;
}

struct ast_expr *
re_ast_plus(struct ast_expr *e)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_PLUS;
	res->u.plus.e = e;
	LOG("-- %s: %p <- %p\n", __func__, (void *)res, (void *)e);
	return res;
}

struct ast_expr *
re_ast_opt(struct ast_expr *e)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_OPT;
	res->u.opt.e = e;
	LOG("-- %s: %p <- %p\n", __func__, (void *)res, (void *)e);
	return res;
}

struct ast_expr *
re_ast_expr_with_count(struct ast_expr *e, struct ast_count count)
{
	struct ast_expr *res = NULL;
	if (count.low > count.high) {
		fprintf(stderr, "ERROR: low > high (%u, %u)\n",
		    count.low, count.high);
		abort();
	}

	if (count.low == 0) {
		if (count.high == 0) {		         /* 0,0 -> empty */
			return re_ast_expr_empty();
		} else if (count.high == 1) {		 /* 0,1 -> ? */
			/* This isn't strictly necessary, but avoids
			 * some unnecessary states and epsilon
			 * transitions. */
			return re_ast_opt(e);
		} else if (count.high == AST_COUNT_UNBOUNDED) { /* 0,_ -> * */
			return re_ast_kleene(e);
		}
	} else if (count.low == 1) {
		if (count.high == AST_COUNT_UNBOUNDED) { /* 1,_ -> + */
			return re_ast_plus(e);
		} else if (count.high == 1) {	         /* 1,1 -> as-is */
			return e;
		}
	}
			
	res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_REPEATED;
	res->u.repeated.e = e;
	res->u.repeated.low = count.low;
	res->u.repeated.high = count.high;
	LOG("-- %s: %p <- %p (%u,%u)\n", __func__, (void *)res,
	    (void *)e, count.low, count.high);
	return res;
}

struct ast_expr *
re_ast_expr_char_class(struct re_char_class_ast *cca,
    const struct ast_pos *start, const struct ast_pos *end)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }

	res->t = AST_EXPR_CLASS;
	res->u.class.cca = cca;
	memcpy(&res->u.class.start, start, sizeof *start);
	memcpy(&res->u.class.end, end, sizeof *end);
	LOG("-- %s: %p <- %p\n", __func__, (void *)res, (void *)cca);
	return res;
}

struct ast_expr *
re_ast_expr_group(struct ast_expr *e)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_GROUP;
	res->u.group.e = e;
	res->u.group.id = (unsigned)-1; /* not yet assigned */
	LOG("-- %s: %p <- %p\n", __func__, (void *)res, (void *)e);
	return res;
}

struct ast_expr *
re_ast_expr_re_flags(enum re_flags pos, enum re_flags neg)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_FLAGS;
	res->u.flags.pos = pos;
	res->u.flags.neg = neg;
	LOG("-- %s: %p <- pos %04x, neg %04x\n", __func__, (void *)res,
	    (unsigned)pos, (unsigned)neg);
	return res;
}



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
	case AST_EXPR_CLASS:
		fprintf(f, "CLASS %p: \n", (void *)n);
		re_char_class_ast_prettyprint(f, n->u.class.cca, indent + IND);
		fprintf(f, "\n");
		break;
	case AST_EXPR_GROUP:
		fprintf(f, "GROUP %p: %u\n", (void *)n, n->u.group.id);
		pp_iter(f, indent + 1*IND, n->u.group.e);
		break;
	case AST_EXPR_FLAGS:
		fprintf(f, "FLAGS %p: pos:(", (void *)n);
		re_flags_prettyprint(f, n->u.flags.pos);
		fprintf(f, "), neg:(");
		re_flags_prettyprint(f, n->u.flags.neg);
		fprintf(f, ")\n");
		break;
	default:
		assert(0);
	}
}

void
re_ast_prettyprint(FILE *f, struct ast_re *ast)
{
	pp_iter(f, 0, ast->expr);
}

static void
re_flags_prettyprint(FILE *f, enum re_flags fl)
{
	const char *sep = "";
	if (fl & RE_ICASE) { fprintf(f, "%sicase", sep); sep = " "; }
	if (fl & RE_TEXT) { fprintf(f, "%stext ", sep); sep = " "; }
	if (fl & RE_MULTI) { fprintf(f, "%smulti ", sep); sep = " "; }
	if (fl & RE_REVERSE) { fprintf(f, "%sreverse ", sep); sep = " "; }
	if (fl & RE_SINGLE) { fprintf(f, "%ssingle ", sep); sep = " "; }
	if (fl & RE_ZONE) { fprintf(f, "%szone ", sep); sep = " "; }
}

struct ast_count
ast_count(unsigned low, const struct ast_pos *start,
    unsigned high, const struct ast_pos *end)
{
	struct ast_count res;
	memset(&res, 0x00, sizeof res);
	res.low = low;
	res.high = high;

	if (start != NULL) { res.start = *start; }
	if (end != NULL) { res.end = *end; }
	return res;
}
