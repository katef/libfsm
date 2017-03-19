/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <fsm/pred.h>

#include "../internal.h"

int
fsm_isend(const struct fsm *fsm, const struct fsm_state *state)
{
	assert(fsm != NULL);
	assert(state != NULL);

	(void) fsm;

	return !!state->end;
}

