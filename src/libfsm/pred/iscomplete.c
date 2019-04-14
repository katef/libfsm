/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <limits.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include <fsm/pred.h>

#include "../internal.h"

int
fsm_iscomplete(const struct fsm *fsm, const struct fsm_state *state)
{
	struct fsm_edge *e;
	struct edge_iter it;
	unsigned n;

	assert(fsm != NULL);
	assert(state != NULL);

	n = 0;

	/* epsilon transitions have no effect on completeness */
	(void) state->epsilons;

	/* TODO: assert state is in fsm->sl */
	for (e = edge_set_first(state->edges, &it); e != NULL; e = edge_set_next(&it)) {
		if (state_set_empty(e->sl)) {
			continue;
		}

		n++;
	}

	return n == FSM_SIGMA_COUNT;
}

