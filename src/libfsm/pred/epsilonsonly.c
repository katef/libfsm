/*
 * Copyright 2008-2017 Katherine Flavel
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
fsm_epsilonsonly(const struct fsm *fsm, fsm_state_t state)
{
	assert(fsm != NULL);
	assert(state < fsm->statecount);

	if (state_set_empty(fsm->states[state].epsilons)) {
		return 0;
	}

	if (!edge_set_empty(fsm->states[state].edges)) {
		return 0;
	}

	return 1;
}

