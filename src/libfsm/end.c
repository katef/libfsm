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

	state->end = !!end;
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

