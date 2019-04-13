/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include <fsm/pred.h>

#include "../internal.h"

int
fsm_epsilonsonly(const struct fsm *fsm, const struct fsm_state *state)
{
	struct fsm_edge *e;
	struct edge_iter it;

	assert(fsm != NULL);
	assert(state != NULL);

	(void) fsm;

	if (state_set_empty(state->epsilons)) {
		return 0;
	}

	for (e = edge_set_first(state->edges, &it); e != NULL; e = edge_set_next(&it)) {
		if (!state_set_empty(e->sl)) {
			return 0;
		}
	}

	return 1;
}

