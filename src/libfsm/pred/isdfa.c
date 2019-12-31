/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include <print/esc.h>

#include <adt/bitmap.h>
#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "../internal.h"

int
fsm_isdfa(const struct fsm *fsm, fsm_state_t state)
{
	fsm_state_t start;
	struct bm bm;

	assert(fsm != NULL);
	assert(state < fsm->statecount);

	/*
	 * DFA must have a start state (and therefore at least one state).
	 */
	if (!fsm_getstart(fsm, &start)) {
		return 0;
	}

	/*
	 * DFA may not have epsilon edges.
	 */
	if (!state_set_empty(fsm->states[state].epsilons)) {
		return 0;
	}

	bm_clear(&bm);

	if (state_hasnondeterminism(fsm, state, &bm)) {
		return 0;
	}

	return 1;
}

