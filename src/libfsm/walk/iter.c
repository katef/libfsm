/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/walk.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "../internal.h"

int
fsm_walk_states(const struct fsm *fsm, void *opaque,
	int (*callback)(const struct fsm *, fsm_state_t, void *))
{
	fsm_state_t i;

	assert(fsm != NULL);
	assert(callback != NULL);

	for (i = 0; i < fsm->statecount; i++) {
		if (!callback(fsm, i, opaque)) {
			return 0;
		}
	}

	return 1;
}

int
fsm_walk_edges(const struct fsm *fsm, void *opaque,
	int (*callback_literal)(const struct fsm *, fsm_state_t, fsm_state_t, char c, void *),
	int (*callback_epsilon)(const struct fsm *, fsm_state_t, fsm_state_t, void *))
{
	fsm_state_t i;

	assert(fsm != NULL);
	assert(callback_literal != NULL);
	assert(callback_epsilon != NULL);

	for (i = 0; i < fsm->statecount; i++) {
		const struct fsm_edge *e;
		struct edge_iter ei;

		for (e = edge_set_first(fsm->states[i]->edges, &ei); e != NULL; e = edge_set_next(&ei)) {
			struct state_iter di;
			fsm_state_t dst;

			for (state_set_reset(e->sl, &di); state_set_next(&di, &dst); ) {
				if (!callback_literal(fsm, i, dst, e->symbol, opaque)) {
					return 0;
				}
			}
		}

		{
			struct state_iter di;
			fsm_state_t dst;

			for (state_set_reset(fsm->states[i]->epsilons, &di); state_set_next(&di, &dst); ) {
				if (!callback_epsilon(fsm, i, dst, opaque)) {
					return 0;
				}
			}
		}
	}

	return 1;
}

