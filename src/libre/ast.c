/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <re/re.h>

#include "class.h"
#include "ast.h"

/*
 * This is a placeholder for a node that has already been freed.
 * Note: this is a single-instance node, which other functions
 * should not modify.
 */
static struct ast_expr the_tombstone;
struct ast_expr *ast_expr_tombstone = &the_tombstone;

struct ast *
ast_new(void)
{
	struct ast *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	/* XXX: not thread-safe */
	the_tombstone.type = AST_EXPR_TOMBSTONE;

	return res;
}

static void
free_iter(struct ast_expr *n)
{
	if (n == NULL) {
		return;
	}
	
	switch (n->type) {
	case AST_EXPR_EMPTY:
	case AST_EXPR_LITERAL:
	case AST_EXPR_ANY:
	case AST_EXPR_FLAGS:
	case AST_EXPR_ANCHOR:
	case AST_EXPR_RANGE:
	case AST_EXPR_NAMED:
		/* these nodes have no subnodes or dynamic allocation */
		break;

	case AST_EXPR_SUBTRACT:
		free_iter(n->u.subtract.a);
		free_iter(n->u.subtract.b);
		break;

	case AST_EXPR_INVERT:
		free_iter(n->u.invert.e);
		break;

	case AST_EXPR_CONCAT: {
		size_t i;

		for (i = 0; i < n->u.concat.count; i++) {
			free_iter(n->u.concat.n[i]);
		}

		free(n->u.concat.n);
		break;
	}

	case AST_EXPR_ALT: {
		size_t i;

		for (i = 0; i < n->u.alt.count; i++) {
			free_iter(n->u.alt.n[i]);
		}
		break;
	}

	case AST_EXPR_REPEATED:
		free_iter(n->u.repeated.e);
		break;

	case AST_EXPR_GROUP:
		free_iter(n->u.group.e);
		break;

	case AST_EXPR_TOMBSTONE:
		/* do not free */
		return;

	default:
		assert(!"unreached");
	}

	free(n);
}

void
ast_free(struct ast *ast)
{
	ast_expr_free(ast->expr);
	free(ast);
}

struct ast_count
ast_make_count(unsigned low, const struct ast_pos *start,
    unsigned high, const struct ast_pos *end)
{
	struct ast_count res;

	memset(&res, 0x00, sizeof res);

	res.low  = low;
	res.high = high;

	if (start != NULL) {
		res.start = *start;
	}
	if (end != NULL) {
		res.end = *end;
	}

	return res;
}

/*
 * Expressions
 */

void
ast_expr_free(struct ast_expr *n)
{
	free_iter(n);
}

struct ast_expr *
ast_make_expr_empty(void)
{
	struct ast_expr *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_EXPR_EMPTY;

	return res;
}

struct ast_expr *
ast_make_expr_concat(void)
{
	struct ast_expr *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_EXPR_CONCAT;
	res->u.concat.alloc = 8; /* arbitrary initial value */
	res->u.concat.count = 0;

	res->u.concat.n = malloc(res->u.concat.alloc * sizeof *res->u.concat.n);
	if (res->u.concat.n == NULL) {
		free(res);
		return NULL;
	}

	return res;
}

int
ast_add_expr_concat(struct ast_expr *cat, struct ast_expr *node)
{
	assert(cat != NULL);
	assert(cat->type == AST_EXPR_CONCAT);

	if (cat->u.concat.count == cat->u.concat.alloc) {
		void *tmp;

		tmp = realloc(cat->u.concat.n, cat->u.concat.alloc * 2 * sizeof *cat->u.concat.n);
		if (tmp == NULL) {
			return 0;
		}

		cat->u.concat.alloc *= 2;
		cat->u.concat.n = tmp;
	}

	cat->u.concat.n[cat->u.concat.count] = node;
	cat->u.concat.count++;

	return 1;
}

struct ast_expr *
ast_make_expr_alt(void)
{
	struct ast_expr *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_EXPR_ALT;
	res->u.alt.alloc = 8; /* arbitrary initial value */
	res->u.alt.count = 0;

	res->u.alt.n = malloc(res->u.alt.alloc * sizeof *res->u.alt.n);
	if (res->u.alt.n == NULL) {
		free(res);
		return NULL;
	}

	return res;
}

int
ast_add_expr_alt(struct ast_expr *cat, struct ast_expr *node)
{
	assert(cat != NULL);
	assert(cat->type == AST_EXPR_ALT);

	if (cat->u.alt.count == cat->u.alt.alloc) {
		void *tmp;

		tmp = realloc(cat->u.alt.n, cat->u.alt.alloc * 2 * sizeof *cat->u.alt.n);
		if (tmp == NULL) {
			return 0;
		}

		cat->u.alt.alloc *= 2;
		cat->u.alt.n = tmp;
	}

	cat->u.alt.n[cat->u.alt.count] = node;
	cat->u.alt.count++;

	return 1;
}

struct ast_expr *
ast_make_expr_literal(char c)
{
	struct ast_expr *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_EXPR_LITERAL;
	res->u.literal.c = c;

	return res;
}

struct ast_expr *
ast_make_expr_any(void)
{
	struct ast_expr *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_EXPR_ANY;

	return res;
}

struct ast_expr *
ast_make_expr_with_count(struct ast_expr *e, struct ast_count count)
{
	struct ast_expr *res = NULL;

	assert(count.low <= count.high);

	if (count.low == 1 && count.high == 1) {
		return e; /* skip the useless REPEATED node */
	}

	/* Applying a count to a start/end anchor is a syntax error. */
	if (e->type == AST_EXPR_ANCHOR) {
		return NULL;
	}

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_EXPR_REPEATED;
	res->u.repeated.e = e;
	res->u.repeated.low = count.low;
	res->u.repeated.high = count.high;

	return res;
}

struct ast_expr *
ast_make_expr_group(struct ast_expr *e)
{
	struct ast_expr *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_EXPR_GROUP;
	res->u.group.e = e;
	res->u.group.id = NO_GROUP_ID; /* not yet assigned */

	return res;
}

struct ast_expr *
ast_make_expr_re_flags(enum re_flags pos, enum re_flags neg)
{
	struct ast_expr *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_EXPR_FLAGS;
	res->u.flags.pos = pos;
	res->u.flags.neg = neg;

	return res;
}

struct ast_expr *
ast_make_expr_anchor(enum ast_anchor_type type)
{
	struct ast_expr *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_EXPR_ANCHOR;
	res->u.anchor.type = type;

	return res;
}

struct ast_expr *
ast_make_expr_subtract(struct ast_expr *a, struct ast_expr *b)
{
	struct ast_expr *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_EXPR_SUBTRACT;
	res->u.subtract.a = a;
	res->u.subtract.b = b;

	return res;
}

struct ast_expr *
ast_make_expr_invert(struct ast_expr *e)
{
	struct ast_expr *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_EXPR_INVERT;
	res->u.invert.e = e;

	return res;
}

struct ast_expr *
ast_make_expr_range(const struct ast_endpoint *from, struct ast_pos start,
    const struct ast_endpoint *to, struct ast_pos end)
{
	struct ast_expr *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	assert(from != NULL);
	assert(to != NULL);

	res->type = AST_EXPR_RANGE;
	res->u.range.from = *from;
	res->u.range.start = start;
	res->u.range.to = *to;
	res->u.range.end = end;

	return res;
}

struct ast_expr *
ast_make_expr_named(class_constructor *ctor)
{
	struct ast_expr *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_EXPR_NAMED;
	res->u.named.ctor = ctor;

	return res;
}

