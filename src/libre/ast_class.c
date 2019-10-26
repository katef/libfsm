/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <re/re.h>

#include "class.h"
#include "ast.h"

struct ast_class *
ast_class_concat(struct ast_class *l, struct ast_class *r)
{
	struct ast_class *res = calloc(1, sizeof(*res));
	if (res == NULL) { return NULL; }
	res->t = AST_CLASS_CONCAT;
	res->u.concat.l = l;
	res->u.concat.r = r;
	return res;
}

struct ast_class *
ast_class_literal(unsigned char c)
{
	struct ast_class *res = calloc(1, sizeof(*res));
	if (res == NULL) { return NULL; }
	res->t = AST_CLASS_LITERAL;
	res->u.literal.c = c;
	return res;
}

struct ast_class *
ast_class_range(const struct ast_range_endpoint *from, struct ast_pos start,
    const struct ast_range_endpoint *to, struct ast_pos end)
{
	struct ast_class *res = calloc(1, sizeof(*res));
	if (res == NULL) { return NULL; }
	assert(from != NULL);
	assert(to != NULL);

	res->t = AST_CLASS_RANGE;
	res->u.range.from = *from;
	res->u.range.start = start;
	res->u.range.to = *to;
	res->u.range.end = end;
	return res;
}

struct ast_class *
ast_class_flags(enum ast_class_flags flags)
{
	struct ast_class *res = calloc(1, sizeof(*res));
	if (res == NULL) { return NULL; }
	res->t = AST_CLASS_FLAGS;
	res->u.flags.f = flags;
	return res;
}

struct ast_class *
ast_class_named(class_constructor *ctor)
{
	struct ast_class *res = calloc(1, sizeof(*res));
	if (res == NULL) { return NULL; }
	res->t = AST_CLASS_NAMED;
	res->u.named.ctor = ctor;
	return res;
}

struct ast_class *
ast_class_subtract(struct ast_class *ast,
    struct ast_class *mask)
{
	struct ast_class *res = calloc(1, sizeof(*res));
	if (res == NULL) { return NULL; }
	res->t = AST_CLASS_SUBTRACT;
	res->u.subtract.ast = ast;
	res->u.subtract.mask = mask;
	return res;
}

static void
free_iter(struct ast_class *n)
{
	assert(n != NULL);
	switch (n->t) {
	case AST_CLASS_CONCAT:
		free_iter(n->u.concat.l);
		free_iter(n->u.concat.r);
		break;
	case AST_CLASS_SUBTRACT:
		free_iter(n->u.subtract.ast);
		free_iter(n->u.subtract.mask);
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

void
ast_class_free(struct ast_class *ast)
{
	free_iter(ast);
}

