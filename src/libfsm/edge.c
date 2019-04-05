/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>
#include <adt/xalloc.h>

#include <fsm/fsm.h>

#include "internal.h"

static int
fsm_addedge(struct fsm_state *from, struct fsm_state *to, enum fsm_edge_type type)
{
	struct fsm_edge *e, new;

	assert(from != NULL);
	assert(to != NULL);

	new.symbol = type;
	e = edge_set_contains(from->edges, &new);
	if (e == NULL) {
		e = malloc(sizeof *e);
		if (e == NULL) {
			return 0;
		}

		e->symbol = type;
		e->sl = state_set_create();

		if (!edge_set_add(from->edges, e)) {
			return 0;
		}
	}

	if (state_set_add(e->sl, to) == NULL) {
		return 0;
	}

	return 1;
}

int
fsm_addedge_epsilon(struct fsm *fsm,
	struct fsm_state *from, struct fsm_state *to)
{
	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);

	(void) fsm;

	return fsm_addedge(from, to, FSM_EDGE_EPSILON);
}

int
fsm_addedge_any(struct fsm *fsm,
	struct fsm_state *from, struct fsm_state *to)
{
	int i;

	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);

	(void) fsm;

	for (i = 0; i <= UCHAR_MAX; i++) {
		if (!fsm_addedge(from, to, i)) {
			return 0;
		}
	}

	return 1;
}

int
fsm_addedge_literal(struct fsm *fsm,
	struct fsm_state *from, struct fsm_state *to,
	char c)
{
	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);

	(void) fsm;

	return fsm_addedge(from, to, (unsigned char) c);
}

struct fsm_edge *
fsm_hasedge(const struct fsm_state *s, int c)
{
	struct fsm_edge *e, search;

	search.symbol = c;
	e = edge_set_contains(s->edges, &search);
	if (e == NULL || state_set_empty(e->sl)) {
		return NULL;
	}

	return e;
}
