/* $Id$ */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <re/re.h>

#include "ast.h"
#include "xalloc.h"

struct lx_mapping {
	struct re *re;
	const char *token;

	struct lx_mapping *next;
};

struct lx_zone {
	struct lx_mapping *ml;

	struct lx_zone *next;
};

struct lx_ast {
	struct lx_zone *zl;
};


struct lx_ast *
ast_new(void)
{
	struct lx_ast *new;

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	return new;
}

struct lx_zone *
ast_addzone(struct lx_ast *ast)
{
	struct lx_zone *new;

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->ml = NULL;

	new->next = ast->zl;
	ast->zl   = new;

	return new;
}

struct lx_mapping *
ast_addmapping(struct lx_zone *z, struct re *re, const char *token)
{
	struct lx_mapping *m;

	assert(z != NULL);
	assert(re != NULL);
	assert(token != NULL);

	for (m = z->ml; m != NULL; m = m->next) {
		assert(m->re != NULL);
		assert(m->token != NULL);

		if (0 == strcmp(m->token, token)) {
			break;
		}
	}

	if (m == NULL) {
		m = malloc(sizeof *m);
		if (m == NULL) {
			return NULL;
		}

		m->re = re_new_empty();
		if (m->re == NULL) {
			free(m);
			return NULL;
		}

		m->token = xstrdup(token);
		if (m->token == NULL) {
			re_free(m->re);
			free(m);
			return NULL;
		}

		m->next = z->ml;
		z->ml   = m;
	}

	if (!re_union(m->re, re)) {
		return NULL;
	}

	return m;
}

