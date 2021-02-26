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

	if (fsm->states[state].end == !!end) {
		return;
	}

	switch (end) {
	case 0:
		assert(fsm->endcount > 0);
		fsm->endcount--;
		fsm->states[state].end = 0;
		break;

	case 1:
		assert(fsm->endcount < FSM_ENDCOUNT_MAX);
		fsm->endcount++;
		fsm->states[state].end = 1;
		break;
	}
}
