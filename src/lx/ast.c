/* $Id$ */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <re/re.h>

#include <fsm/bool.h>

#include <adt/xalloc.h>

#include "ast.h"

#include "libre/internal.h" /* XXX */

struct ast *
ast_new(void)
{
	struct ast *new;

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->zl = NULL;

	return new;
}

struct ast_token *
ast_addtoken(struct ast *ast, const char *s)
{
	struct ast_token *new;

	assert(s != NULL);

	{
		struct ast_token *t;

		for (t = ast->tl; t != NULL; t = t->next) {
			if (0 == strcmp(t->s, s)) {
				return t;
			}
		}
	}

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	/* TODO: could append after new's storage */
	new->s = xstrdup(s);

	new->next = ast->tl;
	ast->tl   = new;

	return new;
}

struct ast_zone *
ast_addzone(struct ast *ast)
{
	struct ast_zone *new;

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->ml = NULL;
	new->re = NULL;

	new->next = ast->zl;
	ast->zl   = new;

	return new;
}

struct ast_mapping *
ast_addmapping(struct ast_zone *z, struct re *re,
	struct ast_token *token, struct ast_zone *to)
{
	struct ast_mapping *m;

	assert(z != NULL);
	assert(re != NULL);

	for (m = z->ml; m != NULL; m = m->next) {
		assert(m->re != NULL);

		if (token == m->token && to == m->to) {
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

		m->token = token;
		m->to    = to;

		m->next = z->ml;
		z->ml   = m;
	}

	/* TODO: re_addcolour(re, m) */

	m->re->fsm = fsm_union(m->re->fsm, re->fsm);
	free(re); /* XXX: abstraction */
	if (m->re->fsm == NULL) {
		return NULL;
	}

	return m;
}

