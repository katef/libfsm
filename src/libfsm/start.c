/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <adt/set.h>

#include <fsm/fsm.h>

#include "internal.h"

void
fsm_setstart(struct fsm *fsm, struct fsm_state *state)
{
	assert(fsm != NULL);

	fsm->start = state;
}

struct fsm_state *
fsm_getstart(const struct fsm *fsm)
{
	assert(fsm != NULL);

	return fsm->start;
}

