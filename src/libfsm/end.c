/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include <adt/set.h>

#include "internal.h"

void
fsm_setend(struct fsm *fsm, fsm_state_t state, int end)
{
	(void) fsm;

	assert(fsm != NULL);
	assert(state < fsm->statecount);

	if (fsm->states[state]->end == !!end) {
		return;
	}

	switch (end) {
	case 0:
		assert(fsm->endcount > 0);
		fsm->endcount--;
		fsm->states[state]->end = 0;
		break;

	case 1:
		assert(fsm->endcount < FSM_ENDCOUNT_MAX);
		fsm->endcount++;
		fsm->states[state]->end = 1;
		break;
	}
}

void
fsm_setendopaque(struct fsm *fsm, void *opaque)
{
	fsm_state_t i;

	assert(fsm != NULL);

	for (i = 0; i < fsm->statecount; i++) {
		if (fsm_isend(fsm, i)) {
			fsm_setopaque(fsm, i, opaque);
		}
	}
}

void
fsm_setopaque(struct fsm *fsm, fsm_state_t state, void *opaque)
{
	(void) fsm;

	assert(fsm != NULL);
	assert(state < fsm->statecount);

	assert(fsm->states[state]->end);

	fsm->states[state]->opaque = opaque;
}

void *
fsm_getopaque(const struct fsm *fsm, fsm_state_t state)
{
	(void) fsm;

	assert(fsm != NULL);
	assert(state < fsm->statecount);

	assert(fsm->states[state]->end);

	return fsm->states[state]->opaque;
}

