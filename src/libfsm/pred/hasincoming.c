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
fsm_hasincoming(const struct fsm *fsm, const struct fsm_state *state)
{
	size_t i;

	assert(fsm != NULL);
	assert(state != NULL);

	for (i = 0; i < fsm->statecount; i++) {
		struct fsm_edge *e;
		struct edge_iter it;

		if (state_set_contains(fsm->states[i]->epsilons, state)) {
			return 1;
		}

		for (e = edge_set_first(fsm->states[i]->edges, &it); e != NULL; e = edge_set_next(&it)) {
			if (e->sl != NULL && state_set_contains(e->sl, state)) {
				return 1;
			}
		}
	}

	return 0;
}

