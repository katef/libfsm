/* $Id$ */

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <fsm/fsm.h>

#include "internal.h"
#include "set.h"

static int
isdfastate(const struct fsm_state *state)
{
	int i;

	/*
	 * DFA may not have epsilon edges.
	 */
	if (state->el != NULL) {
		return 0;
	}

	/*
	 * DFA may not have duplicate edges.
	 */
	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		if (state->edges[i] == NULL) {
			continue;
		}

		if (state->edges[i]->next != NULL) {
			return 0;
		}
	}

	return 1;
}

int
fsm_isdfa(const struct fsm *fsm)
{
	const struct fsm_state *s;

	assert(fsm != NULL);
	assert(fsm->start != NULL);

	if (fsm->sl == NULL) {
		return 0;
	}

	for (s = fsm->sl; s; s = s->next) {
		if (!isdfastate(s)) {
			return 0;
		}
	}

	return 1;
}

