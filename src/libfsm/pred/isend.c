/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include "../internal.h"

int
fsm_isend(const struct fsm *fsm, fsm_state_t state)
{
	assert(fsm != NULL);
	assert(state < fsm->statecount);

	return !!fsm->states[state]->end;
}

