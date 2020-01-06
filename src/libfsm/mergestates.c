/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "internal.h"

int
fsm_mergestates(struct fsm *fsm, fsm_state_t a, fsm_state_t b,
	fsm_state_t *q)
{
	struct fsm_edge e;
	struct edge_iter it;
	fsm_state_t i;

	assert(fsm != NULL);
	assert(a < fsm->statecount);
	assert(b < fsm->statecount);

	/* edges from b */
	if (!state_set_copy(&fsm->states[a].epsilons, fsm->opt->alloc, fsm->states[b].epsilons)) {
		return 0;
	}
	if (!edge_set_copy(&fsm->states[a].edges, fsm->opt->alloc, fsm->states[b].edges)) {
		return 0;
	}

	/* edges to b */
	for (i = 0; i < fsm->statecount; i++) {
		if (state_set_contains(fsm->states[i].epsilons, b)) {
			state_set_remove(&fsm->states[i].epsilons, b);

			if (!fsm_addedge_epsilon(fsm, i, a)) {
				return 0;
			}
		}

		for (edge_set_reset(fsm->states[i].edges, &it); edge_set_next(&it, &e); ) {
			if (e.state != b) {
				continue;
			}

			if (!fsm_addedge_literal(fsm, i, a, e.symbol)) {
				return 0;
			}
		}

		edge_set_remove_state(&fsm->states[i].edges, b);
	}

	fsm_removestate(fsm, b);

	if (q != NULL) {
		*q = a;
	}

	return 1;
}

