/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <adt/set.h>

#include <fsm/pred.h>

#include "../internal.h"

int
fsm_epsilonsonly(const struct fsm *fsm, const struct fsm_state *state)
{
	struct fsm_edge *e, s;
	struct set_iter it;

	assert(fsm != NULL);
	assert(state != NULL);

	(void) fsm;

	s.symbol = FSM_EDGE_EPSILON;
	e = set_contains(state->edges, &s);
	if (e == NULL || set_empty(e->sl)) {
		return 0;
	}

	for (e = set_first(state->edges, &it); e != NULL; e = set_next(&it)) {
		if (e->symbol == FSM_EDGE_EPSILON) {
			continue;
		}

		if (!set_empty(e->sl)) {
			return 0;
		}
	}

	return 1;
}

