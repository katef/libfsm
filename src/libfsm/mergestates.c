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
	struct fsm_edge *e;
	struct edge_iter it;
	fsm_state_t i;

	assert(fsm != NULL);
	assert(a < fsm->statecount);
	assert(b < fsm->statecount);

	/* edges from b */
	{
		struct state_iter jt;
		fsm_state_t s;

		for (state_set_reset(fsm->states[b]->epsilons, &jt); state_set_next(&jt, &s); ) {
			if (!fsm_addedge_epsilon(fsm, a, s)) {
				return 0;
			}
		}
	}
	for (e = edge_set_first(fsm->states[b]->edges, &it); e != NULL; e = edge_set_next(&it)) {
		struct state_iter jt;
		fsm_state_t s;

		for (state_set_reset(e->sl, &jt); state_set_next(&jt, &s); ) {
			if (!fsm_addedge_literal(fsm, a, s, e->symbol)) {
				return 0;
			}
		}
	}

	/* edges to b */
	for (i = 0; i < fsm->statecount; i++) {
		if (state_set_contains(fsm->states[i]->epsilons, b)) {
			state_set_remove(fsm->states[i]->epsilons, b);

			if (!fsm_addedge_epsilon(fsm, i, a)) {
				return 0;
			}
		}

		for (e = edge_set_first(fsm->states[i]->edges, &it); e != NULL; e = edge_set_next(&it)) {
			state_set_remove(e->sl, b);

			if (state_set_contains(e->sl, b)) {
				if (!fsm_addedge_literal(fsm, i, a, e->symbol)) {
					return 0;
				}
			}
		}
	}

	fsm_removestate(fsm, b);

	if (q != NULL) {
		*q = a;
	}

	return 1;
}

