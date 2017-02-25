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
	struct fsm_edge *e, s;
	struct set_iter it;

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
	s.symbol = FSM_EDGE_EPSILON;
	e = set_contains(state->edges, &s);
	if (e != NULL && !set_empty(e->sl)) {
		return 0;
	}

	/*
	 * DFA may not have duplicate edges.
	 */
	for (e = set_first(state->edges, &it); e != NULL; e = set_next(&it)) {
		if (e->symbol > UCHAR_MAX) {
			break;
		}

		if (set_empty(e->sl)) {
			continue;
		}

		(void) set_first(e->sl, &it);
		if (set_hasnext(&it)) {
			return 0;
		}
	}

	return 1;
}

