/* $Id$ */

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <adt/set.h>

#include <fsm/pred.h>

#include "internal.h"

/* TODO: rename fsm_isdfa */
int
fsm_isdfastate(const struct fsm *fsm, const struct fsm_state *state)
{
	int i;

	assert(fsm != NULL);

	(void) fsm;

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

int
fsm_isdfa(const struct fsm *fsm)
{
	const struct fsm_state *s;

	assert(fsm != NULL);

	if (fsm->sl == NULL) {
		return 0;
	}

	if (fsm->start == NULL) {
		return 0;
	}

	for (s = fsm->sl; s != NULL; s = s->next) {
		if (!fsm_isdfastate(fsm, s)) {
			return 0;
		}
	}

	return 1;
}

