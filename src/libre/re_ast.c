#include "re_ast.h"

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
	case AST_EXPR_LITERAL:
		break;
	default:
		assert(0);
	}
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
re_ast_expr_literal(char c, struct ast_expr *r)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_LITERAL;
	res->u.literal.l.c = c;
	res->u.literal.n = r;
	LOG("-- %s: %p <- %c, %p\n",
	    __func__, (void *)res, c, (void *)r);
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
	INDENT(f, indent);

	switch (n->t) {
	case AST_EXPR_LITERAL:
		fprintf(f, "LITERAL %p: '%c'\n", (void *)n, n->u.literal.l.c);
		pp_iter(f, indent, n->u.literal.n);
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
