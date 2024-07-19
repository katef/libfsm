/*
 * Copyright 2024 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <stdlib.h>
#include <fsm/fsm.h>
#include <fsm/alloc.h>

#include <assert.h>

#include "internal.h"

bool
fsm_vacuum(struct fsm *fsm)
{
	assert(fsm != NULL);
	assert(fsm->statecount <= fsm->statealloc);
	size_t nceil = fsm->statealloc;

	while ((fsm->statecount < nceil/2) && nceil > 1) {
		nceil /= 2;
	}

	if (nceil == fsm->statealloc) {
		return true;
	}

	struct fsm_state *nstates = f_realloc(fsm->alloc,
	    fsm->states, nceil * sizeof(nstates[0]));
	if (nstates == NULL) {
		return false;
	}

	fsm->states = nstates;
	fsm->statealloc = nceil;

	return true;
}
