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
	if (state->edges[FSM_EDGE_EPSILON].sl != NULL) {
		return 0;
	}

	/*
	 * DFA may not have duplicate edges.
	 */
	for (i = 0; i <= UCHAR_MAX; i++) {
		if (state->edges[i].sl == NULL) {
			continue;
		}

		if (state->edges[i].sl->next != NULL) {
			return 0;
		}
	}

	return 1;
}

