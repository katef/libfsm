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
	const struct fsm_state *s;

	assert(fsm != NULL);
	assert(state != NULL);

	if (pred_known(state, PRED_HASINCOMING)) {
		return pred_get(state, PRED_HASINCOMING);
	}

	for (s = fsm->sl; s != NULL; s = s->next) {
		struct fsm_edge *e;
		struct edge_iter it;

		for (e = edge_set_first(s->edges, &it); e != NULL; e = edge_set_next(&it)) {
			if (state_set_contains(e->sl, state)) {
				pred_set((void *) state, PRED_HASINCOMING, 1);
				return 1;
			}
		}
	}

	pred_set((void *) state, PRED_HASINCOMING, 0);
	return 0;
}

