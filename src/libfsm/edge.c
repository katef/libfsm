/* $Id$ */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include <adt/set.h>
#include <adt/xalloc.h>

#include <fsm/fsm.h>

#include "internal.h"

static struct state_set *
fsm_addedge(struct fsm_state *from, struct fsm_state *to,
	enum fsm_edge_type type)
{
	struct fsm_edge *edge;

	assert(from != NULL);
	assert(to != NULL);

	edge = &from->edges[type];

	return set_addstate(&edge->sl, to);
}

struct fsm_edge *
fsm_addedge_epsilon(struct fsm *fsm,
	struct fsm_state *from, struct fsm_state *to)
{
	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);

	(void) fsm;

	if (!fsm_addedge(from, to, FSM_EDGE_EPSILON)) {
		return NULL;
	}

	return &from->edges[FSM_EDGE_EPSILON];
}

struct fsm_edge *
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
			return NULL;
		}
	}

	return &from->edges[i];
}

struct fsm_edge *
fsm_addedge_literal(struct fsm *fsm,
	struct fsm_state *from, struct fsm_state *to,
	char c)
{
	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);

	(void) fsm;

	if (!fsm_addedge(from, to, (unsigned char) c)) {
		return NULL;
	}

	return &from->edges[(unsigned char) c];
}

