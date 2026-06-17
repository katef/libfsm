/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include <re/re.h>

#include <fsm/bool.h>

#include <adt/xalloc.h>

#include "libfsm/internal.h" /* XXX */

#include "lx.h"
#include "ast.h"

struct ast *
ast_new(void)
{
	struct ast *new;

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->zl = NULL;
	new->tl = NULL;

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
ast_addzone(struct ast *ast, struct ast_zone *parent)
{
	struct ast_zone *new;

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->ml   = NULL;
	new->fsm  = NULL;
	new->vl   = NULL;

	new->parent = parent;

	new->next = ast->zl;
	ast->zl   = new;

	return new;
}

struct ast_mapping *
ast_addmapping(struct ast_zone *z, struct fsm *fsm,
	struct ast_token *token, struct ast_zone *to)
{
	struct ast_mapping *m;

	assert(z != NULL);
	assert(fsm != NULL);

	for (m = z->ml; m != NULL; m = m->next) {
		assert(m->fsm != NULL);

		if (token == m->token && to == m->to) {
			break;
		}
	}

	if (m == NULL) {
		m = malloc(sizeof *m);
		if (m == NULL) {
			return NULL;
		}

		m->fsm = fsm_new(fsm->alloc);
		if (m->fsm == NULL) {
			free(m);
			return NULL;
		}

		m->token    = token;
		m->to       = to;

		m->next = z->ml;
		z->ml   = m;
	}

	/* TODO: re_addcolour(fsm, m) */

	m->fsm = fsm_union(m->fsm, fsm, NULL);
	if (m->fsm == NULL) {
		return NULL;
	}

	return m;
}

#define DEF_MAPPINGS 4
static fsm_end_id_t mapping_count = 0;
static fsm_end_id_t mapping_ceil = DEF_MAPPINGS;
static struct ast_mapping **mappings = NULL;

int
ast_setendmapping(struct fsm *fsm, struct ast_mapping *m)
{
	const fsm_end_id_t id = mapping_count;
	fsm_end_id_t nceil = 0;

	lx_mutex_lock();
	if (mappings == NULL) {
		nceil = DEF_MAPPINGS;
	} else if (mapping_count == mapping_ceil) {
		nceil = 2 * mapping_ceil;
		assert(nceil > 0);
	}

	if (nceil > 0) {
		struct ast_mapping **nmappings;
		nmappings = realloc(mappings,
		    nceil * sizeof(nmappings[0]));
		if (nmappings == NULL) {
			lx_mutex_unlock();
			return 0;
		}
		mappings = nmappings;
		mapping_ceil = nceil;
	}

	lx_mutex_unlock();

	if (fsm_setendid(fsm, id)) {
		if (LOG()) {
			fprintf(stderr, "ast_setendmapping: saving mapping %p at mappings[%d]\n",
			    (void *)m, id);
		}
		mappings[id] = m;
		mapping_count++;
		return 1;
	}
	return 0;
}

struct ast_mapping *
ast_getendmappingbyendid(fsm_end_id_t id)
{
	struct ast_mapping *res;

	lx_mutex_lock();
	assert(id < mapping_count);
	res = mappings[id];
	lx_mutex_unlock();

	return res;
}

struct ast_mapping *
ast_getendmapping(const struct fsm *fsm, fsm_state_t s)
{
	fsm_end_id_t *ids;
	size_t count;
	struct ast_mapping *m;
	int res;

	if (!fsm_isend(fsm, s)) {
		errno = EINVAL;
		return NULL;
	}

	count = fsm_endid_count(fsm, s);
	if (count == 0) {
		errno = EINVAL;
		return NULL;
	}

	ids = malloc(count * sizeof *ids);
	if (ids == NULL) {
		return NULL;
	}

	res = fsm_endid_get(fsm, s, count, ids);
	assert(res == 1);

	m = ast_getendmappingbyendid(ids[0]);

	if (LOG()) {
		fprintf(stderr, "ast_getendmapping: got mapping %p mappings[%d]\n",
		    (void *) m, ids[0]);
	}

	free(ids);

	return m;
}

