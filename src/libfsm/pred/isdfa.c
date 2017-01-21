/* $Id$ */

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <adt/set.h>

#include <fsm/pred.h>

#include "../internal.h"

int
fsm_isdfa(const struct fsm *fsm, const struct fsm_state *state)
{
	int i;

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
	if (!set_empty(state->edges[FSM_EDGE_EPSILON].sl)) {
		return 0;
	}

	/*
	 * DFA may not have duplicate edges.
	 */
	for (i = 0; i <= UCHAR_MAX; i++) {
		struct set_iter it;

		if (set_empty(state->edges[i].sl)) {
			continue;
		}

		(void)set_first(state->edges[i].sl, &it);
		if (set_hasnext(&it)) {
			return 0;
		}
	}

	return 1;
}

