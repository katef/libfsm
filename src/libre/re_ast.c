#include "re_ast.h"

#include <string.h>

struct ast_re *
re_ast_new(void)
{
	struct ast_re *res = calloc(1, sizeof(*res));
	return res;
}

static void
free_iter(struct ast_expr *n)
{
	if (n == NULL) { return; }
	
	switch (n->t) {
	/* These nodes have no subnodes or dynamic allocation */
	case AST_EXPR_EMPTY:
	case AST_EXPR_LITERAL:
	case AST_EXPR_ANY:
	case AST_EXPR_MANY:
	case AST_EXPR_FLAGS:
	case AST_EXPR_CHAR_TYPE:
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
	case AST_EXPR_CHAR_CLASS:
		re_char_class_ast_free(n->u.char_class.cca);
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
	return res;
}

struct ast_expr *
re_ast_expr_any(void)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_ANY;

	return res;
}

struct ast_expr *
re_ast_expr_many(void)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_MANY;

	return res;
}

struct ast_expr *
re_ast_kleene(struct ast_expr *e)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_KLEENE;
	res->u.kleene.e = e;
	return res;
}

struct ast_expr *
re_ast_plus(struct ast_expr *e)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_PLUS;
	res->u.plus.e = e;
	return res;
}

struct ast_expr *
re_ast_opt(struct ast_expr *e)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_OPT;
	res->u.opt.e = e;
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
	return res;
}

struct ast_expr *
re_ast_expr_char_class(struct re_char_class_ast *cca,
    const struct ast_pos *start, const struct ast_pos *end)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }

	res->t = AST_EXPR_CHAR_CLASS;
	res->u.char_class.cca = cca;
	memcpy(&res->u.char_class.start, start, sizeof *start);
	memcpy(&res->u.char_class.end, end, sizeof *end);
	return res;
}

struct ast_expr *
re_ast_expr_char_type(enum ast_char_type_id id)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }

	res->t = AST_EXPR_CHAR_TYPE;
	res->u.char_type.id = id;
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
	return res;
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
