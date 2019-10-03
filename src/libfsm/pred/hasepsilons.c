/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "../internal.h"

int
fsm_hasepsilons(const struct fsm *fsm, fsm_state_t state)
{
	assert(fsm != NULL);
	assert(state < fsm->statecount);

	return !state_set_empty(fsm->states[state]->epsilons);
}

