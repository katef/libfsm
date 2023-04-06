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
		struct edge_group_iter it;
		struct edge_group_iter_info info;

		if (state_set_contains(fsm->states[i].epsilons, state)) {
			return 1;
		}

		edge_set_group_iter_reset(fsm->states[i].edges, EDGE_GROUP_ITER_ALL, &it);
		while (edge_set_group_iter_next(&it, &info)) {
			if (info.to == state) {
				return 1;
			}
		}
	}

	return 0;
}

