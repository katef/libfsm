/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include "internal.h"

void
fsm_setend(struct fsm *fsm, struct fsm_state *state, int end)
{
	(void) fsm;

	assert(fsm != NULL);
	assert(state != NULL);

	if (state->end == !!end) {
		return;
	}

	switch (end) {
	case 0:
		assert(fsm->endcount > 0);
		fsm->endcount--;
		state->end = 0;
		break;

	case 1:
		assert(fsm->endcount < FSM_ENDCOUNT_MAX);
		fsm->endcount++;
		state->end = 1;
		break;
	}
}

void
fsm_setendopaque(struct fsm *fsm, void *opaque)
{
	size_t i;

	assert(fsm != NULL);

	for (i = 0; i < fsm->statecount; i++) {
		if (fsm_isend(fsm, fsm->states[i])) {
			fsm_setopaque(fsm, fsm->states[i], opaque);
		}
	}
}

void
fsm_setopaque(struct fsm *fsm, struct fsm_state *state, void *opaque)
{
	(void) fsm;

	assert(fsm != NULL);
	assert(state != NULL);

	assert(state->end);

	state->opaque = opaque;
}

void *
fsm_getopaque(const struct fsm *fsm, const struct fsm_state *state)
{
	(void) fsm;

	assert(fsm != NULL);
	assert(state != NULL);

	assert(state->end);

	return state->opaque;
}

