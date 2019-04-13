/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include <fsm/pred.h>

#include "../internal.h"

int
fsm_isdfa(const struct fsm *fsm, const struct fsm_state *state)
{
	struct fsm_edge *e;
	struct edge_iter it;

	assert(fsm != NULL);

	/*
	 * DFA must have a start state (and therfore at least one state).
	 */
	if (fsm_getstart(fsm) == NULL) {
		return 0;
	}

	/*
	 * DFA may not have epsilon edges.
	 */
	if (!state_set_empty(state->epsilons)) {
		return 0;
	}

	/*
	 * DFA may not have duplicate edges.
	 */
	for (e = edge_set_first(state->edges, &it); e != NULL; e = edge_set_next(&it)) {
		struct state_iter jt;

		if (state_set_empty(e->sl)) {
			continue;
		}

		(void) state_set_first(e->sl, &jt);
		if (state_set_hasnext(&jt)) {
			return 0;
		}
	}

	return 1;
}

