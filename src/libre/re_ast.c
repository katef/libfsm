#include "re_ast.h"

#include <string.h>

/* This is a placeholder for a node that has already been freed. */
static struct ast_expr the_tombstone;

struct ast_re *
re_ast_new(void)
{
	struct ast_re *res = calloc(1, sizeof(*res));

	the_tombstone.t = AST_EXPR_TOMBSTONE;

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
	case AST_EXPR_FLAGS:
	case AST_EXPR_ANCHOR:
		break;

	case AST_EXPR_CONCAT:
		free_iter(n->u.concat.l);
		free_iter(n->u.concat.r);
		break;
	case AST_EXPR_CONCAT_N:
	{
		size_t i;
		for (i = 0; i < n->u.concat_n.count; i++) {
			free_iter(n->u.concat_n.n[i]);
		}
		break;
	}
	case AST_EXPR_ALT:
		free_iter(n->u.alt.l);
		free_iter(n->u.alt.r);
		break;
	case AST_EXPR_ALT_N:
	{
		size_t i;
		for (i = 0; i < n->u.alt_n.count; i++) {
			free_iter(n->u.alt_n.n[i]);
		}
		break;
	}
	case AST_EXPR_REPEATED:
		free_iter(n->u.repeated.e);
		break;
	case AST_EXPR_CHAR_CLASS:
		re_char_class_ast_free(n->u.char_class.cca);
		break;
	case AST_EXPR_GROUP:
		free_iter(n->u.group.e);
		break;

	case AST_EXPR_TOMBSTONE:
		/* do not free */
		return;

	default:
		assert(0);
	}

	free(n);
}

void
re_ast_free(struct ast_re *ast)
{
	re_ast_expr_free(ast->expr);
	free(ast);
}

void
re_ast_expr_free(struct ast_expr *n)
{
	free_iter(n);
}

struct ast_expr *
re_ast_expr_empty(void)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_EMPTY;
	res->flags = RE_AST_FLAG_NULLABLE;
	return res;
}

/* Note: this returns a single-instance node, which
 * other functions should not modify. */
struct ast_expr *
re_ast_expr_tombstone(void)
{
	return &the_tombstone;
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
re_ast_expr_concat_n(size_t count)
{
	struct ast_expr *res;
	size_t size = sizeof(*res) + (count-1)*sizeof(struct ast_expr *);
	res = calloc(1, size);
	if (res == NULL) { return res; }
	res->t = AST_EXPR_CONCAT_N;
	res->u.concat_n.count = count;
	/* cells are initialized by caller */
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
re_ast_expr_alt_n(size_t count)
{
	struct ast_expr *res;
	size_t size = sizeof(*res) + (count-1)*sizeof(struct ast_expr *);
	res = calloc(1, size);
	if (res == NULL) { return res; }
	res->t = AST_EXPR_ALT_N;
	res->u.alt_n.count = count;
	/* cells are initialized by caller */
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
re_ast_expr_with_count(struct ast_expr *e, struct ast_count count)
{
	struct ast_expr *res = NULL;
	if (count.low > count.high) {
		fprintf(stderr, "ERROR: low > high (%u, %u)\n",
		    count.low, count.high);
		abort();
	}

	if (count.low == 1 && count.high == 1) {
		return e;	/* skip the useless REPEATED node */
	}

	/* Applying a count to a start/end anchor is a syntax error. */
	if (e->t == AST_EXPR_ANCHOR) { return NULL; }

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
re_ast_expr_group(struct ast_expr *e)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_GROUP;
	res->u.group.e = e;
	res->u.group.id = NO_GROUP_ID; /* not yet assigned */
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

struct ast_expr *
re_ast_expr_anchor(enum re_ast_anchor_type t)
{
	struct ast_expr *res = calloc(1, sizeof(*res));
	if (res == NULL) { return res; }
	res->t = AST_EXPR_ANCHOR;
	res->u.anchor.t = t;
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
