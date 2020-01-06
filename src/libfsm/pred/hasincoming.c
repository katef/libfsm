/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "../internal.h"

int
fsm_hasincoming(const struct fsm *fsm, fsm_state_t state)
{
	fsm_state_t i;

	assert(fsm != NULL);
	assert(state < fsm->statecount);

	for (i = 0; i < fsm->statecount; i++) {
		struct fsm_edge e;
		struct edge_iter it;

		if (state_set_contains(fsm->states[i].epsilons, state)) {
			return 1;
		}

		for (edge_set_reset(fsm->states[i].edges, &it); edge_set_next(&it, &e); ) {
			if (e.state == state) {
				return 1;
			}
		}
	}

	return 0;
}

