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
	LOG("%s: %p\n", __func__, (void *)n);
	if (n == NULL) { return; }
	
	switch (n->t) {
	/* These nodes have no subnodes or dynamic allocation */
	case AST_EXPR_EMPTY:
	case AST_EXPR_LITERAL:
	case AST_EXPR_ANY:
	case AST_EXPR_MANY:
		break;

	case AST_EXPR_CONCAT:
		free_iter(n->u.concat.l);
		free_iter(n->u.concat.r);
		break;
	case AST_EXPR_ALT:
		free_iter(n->u.alt.l);
		free_iter(n->u.alt.r);
		break;
	case AST_EXPR_ATOM:
		free_iter(n->u.atom.e);
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
	res->u.literal.l.c = c;
	LOG("-- %s: %p <- %c\n",
	    __func__, (void *)res, c);
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

static const char *
atom_flag_str(enum ast_atom_count_flag f)
{
	switch (f) {
	case AST_ATOM_COUNT_FLAG_KLEENE: return "KLEENE";
	case AST_ATOM_COUNT_FLAG_PLUS: return "PLUS";
	case AST_ATOM_COUNT_FLAG_ONE: return "ONE";
	default: return "<matchfail>";
	}
}

struct ast_expr *
re_ast_expr_atom(struct ast_expr *e, enum ast_atom_count_flag flag)
{
	if (flag == AST_ATOM_COUNT_FLAG_ONE) { return e; }

	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_ATOM;
	res->u.atom.e = e;
	res->u.atom.flag = flag;

	LOG("-- %s: %p <- %p, %s\n",
	    __func__, (void *)res,
	    (void *)e, atom_flag_str(flag));
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
		fprintf(f, "LITERAL %p: '%c'\n", (void *)n, n->u.literal.l.c);
		break;
	case AST_EXPR_ANY:
		fprintf(f, "ANY %p:\n", (void *)n);
		break;
	case AST_EXPR_MANY:
		fprintf(f, "MANY %p:\n", (void *)n);
		break;
	case AST_EXPR_ATOM:	/* FIXME: name */
		fprintf(f, "ATOM %p: %s\n",
		    (void *)n, atom_flag_str(n->u.atom.flag));
		pp_iter(f, indent + 1*IND, n->u.atom.e);
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
