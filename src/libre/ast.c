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

/* This is a placeholder for a node that has already been freed. */
static struct ast_expr the_tombstone;

struct ast *
ast_new(void)
{
	struct ast *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	the_tombstone.type = AST_EXPR_TOMBSTONE;

	return res;
}

static void
class_free_iter(struct ast_class *n)
{
	assert(n != NULL);

	switch (n->type) {
	case AST_CLASS_CONCAT:
		class_free_iter(n->u.concat.l);
		class_free_iter(n->u.concat.r);
		break;

	case AST_CLASS_SUBTRACT:
		class_free_iter(n->u.subtract.ast);
		class_free_iter(n->u.subtract.mask);
		break;

	case AST_CLASS_LITERAL:
	case AST_CLASS_RANGE:
	case AST_CLASS_NAMED:
	case AST_CLASS_FLAGS:
		break;

	default:
		assert(!"unreached");
	}

	free(n);	
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
		/* these nodes have no subnodes or dynamic allocation */
		break;

	case AST_EXPR_CONCAT:
		free_iter(n->u.concat.l);
		free_iter(n->u.concat.r);
		break;

	case AST_EXPR_CONCAT_N: {
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

	case AST_EXPR_ALT_N: {
		size_t i;

		for (i = 0; i < n->u.alt_n.count; i++) {
			free_iter(n->u.alt_n.n[i]);
		}
		break;
	}

	case AST_EXPR_REPEATED:
		free_iter(n->u.repeated.e);
		break;

	case AST_EXPR_CLASS:
		class_free_iter(n->u.class.class);
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

void
ast_expr_free(struct ast_expr *n)
{
	free_iter(n);
}

struct ast_expr *
ast_expr_empty(void)
{
	struct ast_expr *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_EXPR_EMPTY;
	res->flags = RE_AST_FLAG_NULLABLE;

	return res;
}

/* Note: this returns a single-instance node, which
 * other functions should not modify. */
struct ast_expr *
ast_expr_tombstone(void)
{
	return &the_tombstone;
}

struct ast_expr *
ast_expr_concat(struct ast_expr *l, struct ast_expr *r)
{
	struct ast_expr *res;

	assert(l != NULL);
	assert(r != NULL);

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_EXPR_CONCAT;
	res->u.concat.l = l;
	res->u.concat.r = r;

	return res;
}

struct ast_expr *
ast_expr_concat_n(size_t count)
{
	struct ast_expr *res;
	size_t size = sizeof *res + (count - 1) * sizeof (struct ast_expr *);

	res = calloc(1, size);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_EXPR_CONCAT_N;
	res->u.concat_n.count = count;

	/* cells are initialized by caller */

	return res;
}

struct ast_expr *
ast_expr_alt(struct ast_expr *l, struct ast_expr *r)
{
	struct ast_expr *res;

	assert(l != NULL);
	assert(r != NULL);

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return res;
	}

	res->type = AST_EXPR_ALT;
	res->u.alt.l = l;
	res->u.alt.r = r;

	return res;
}

struct ast_expr *
ast_expr_alt_n(size_t count)
{
	struct ast_expr *res;
	size_t size = sizeof *res + (count - 1) * sizeof (struct ast_expr *);

	res = calloc(1, size);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_EXPR_ALT_N;
	res->u.alt_n.count = count;

	/* cells are initialized by caller */

	return res;
}

struct ast_expr *
ast_expr_literal(char c)
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
ast_expr_any(void)
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
ast_expr_with_count(struct ast_expr *e, struct ast_count count)
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
ast_expr_class(struct ast_class *class,
    const struct ast_pos *start, const struct ast_pos *end)
{
	struct ast_expr *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return res;
	}

	res->type = AST_EXPR_CLASS;
	res->u.class.class = class;

	memcpy(&res->u.class.start, start, sizeof *start);
	memcpy(&res->u.class.end, end, sizeof *end);

	return res;
}

struct ast_expr *
ast_expr_group(struct ast_expr *e)
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
ast_expr_re_flags(enum re_flags pos, enum re_flags neg)
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
ast_expr_anchor(enum ast_anchor_type type)
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

struct ast_count
ast_count(unsigned low, const struct ast_pos *start,
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

struct ast_class *
ast_class_concat(struct ast_class *l, struct ast_class *r)
{
	struct ast_class *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_CLASS_CONCAT;
	res->u.concat.l = l;
	res->u.concat.r = r;

	return res;
}

struct ast_class *
ast_class_literal(unsigned char c)
{
	struct ast_class *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_CLASS_LITERAL;
	res->u.literal.c = c;

	return res;
}

struct ast_class *
ast_class_range(const struct ast_range_endpoint *from, struct ast_pos start,
    const struct ast_range_endpoint *to, struct ast_pos end)
{
	struct ast_class *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	assert(from != NULL);
	assert(to != NULL);

	res->type = AST_CLASS_RANGE;
	res->u.range.from = *from;
	res->u.range.start = start;
	res->u.range.to = *to;
	res->u.range.end = end;

	return res;
}

struct ast_class *
ast_class_flags(enum ast_class_flags flags)
{
	struct ast_class *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_CLASS_FLAGS;
	res->u.flags.f = flags;

	return res;
}

struct ast_class *
ast_class_named(class_constructor *ctor)
{
	struct ast_class *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_CLASS_NAMED;
	res->u.named.ctor = ctor;

	return res;
}

struct ast_class *
ast_class_subtract(struct ast_class *ast,
    struct ast_class *mask)
{
	struct ast_class *res;

	res = calloc(1, sizeof *res);
	if (res == NULL) {
		return NULL;
	}

	res->type = AST_CLASS_SUBTRACT;
	res->u.subtract.ast = ast;
	res->u.subtract.mask = mask;

	return res;
}

