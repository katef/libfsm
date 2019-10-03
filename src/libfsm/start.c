/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include <adt/set.h>

#include "internal.h"

void
fsm_clearstart(struct fsm *fsm)
{
	assert(fsm != NULL);

	fsm->hasstart = 0;
}

void
fsm_setstart(struct fsm *fsm, fsm_state_t state)
{
	assert(fsm != NULL);
	assert(state < fsm->statecount);

	fsm->start = state;
	fsm->hasstart = 1;
}

int
fsm_getstart(const struct fsm *fsm, fsm_state_t *start)
{
	assert(fsm != NULL);
	assert(start != NULL);

	if (!fsm->hasstart) {
		return 0;
	}

	assert(fsm->start < fsm->statecount);

	*start = fsm->start;
	return 1;
}

