/*
 * Copyright 2019 Katherine Flavel
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
fsm_hasepsilons(const struct fsm *fsm, const struct fsm_state *state)
{
	struct fsm_edge *e, s;

	assert(fsm != NULL);
	assert(state != NULL);

	(void) fsm;

	s.symbol = FSM_EDGE_EPSILON;
	e = edge_set_contains(state->edges, &s);
	if (e == NULL || state_set_empty(e->sl)) {
		return 0;
	}

	return 1;
}

