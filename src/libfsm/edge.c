/* $Id$ */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include <fsm/fsm.h>

#include <adt/xalloc.h>

#include "internal.h"
#include "set.h"

static struct fsm_edge *fsm_addedge(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to, struct fsm_edge *edge) {
	assert(from != NULL);
	assert(to != NULL);
	assert(edge != NULL);

	(void) fsm;

	if (!set_addstate(&edge->sl, to)) {
		return NULL;
	}

	return edge;
}

struct fsm_edge *
fsm_addedge_epsilon(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to)
{
	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);

	return fsm_addedge(fsm, from, to, &from->edges[FSM_EDGE_EPSILON]);
}

struct fsm_edge *
fsm_addedge_any(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to)
{
	int i;

	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);

	for (i = 0; i <= UCHAR_MAX; i++) {
		if (!fsm_addedge(fsm, from, to, &from->edges[i])) {
			return 0;
		}
	}

	return &from->edges[i];
}

struct fsm_edge *
fsm_addedge_literal(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to,
	char c)
{
	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);

	return fsm_addedge(fsm, from, to, &from->edges[(unsigned char) c]);
}

