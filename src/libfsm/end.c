/* $Id$ */

#include <assert.h>
#include <stdlib.h>

#include <adt/set.h>

#include <fsm/fsm.h>

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
fsm_setopaque(struct fsm *fsm, struct fsm_state *state, void *opaque)
{
	(void) fsm;

	assert(fsm != NULL);
	assert(state != NULL);

	assert(state->end);

	state->opaque = opaque;
}

void *
fsm_getopaque(struct fsm *fsm, struct fsm_state *state)
{
	(void) fsm;

	assert(fsm != NULL);
	assert(state != NULL);

	assert(state->end);

	return state->opaque;
}

