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

enum fsm_vacuum_res
fsm_vacuum(struct fsm *fsm)
{
	assert(fsm != NULL);
	assert(fsm->statecount <= fsm->statealloc);
	size_t nceil = fsm->statealloc;

	while ((fsm->statecount < nceil/2) && nceil > 1) {
		nceil /= 2;
	}

	if (nceil == fsm->statealloc) {
		return FSM_VACUUM_NO_CHANGE;
	}

	struct fsm_state *nstates = f_realloc(fsm->opt->alloc,
	    fsm->states, nceil * sizeof(nstates[0]));
	if (nstates == NULL) {
		return FSM_VACUUM_ERROC_REALLOC;
	}

	fsm->states = nstates;
	fsm->statealloc = nceil;

	return FSM_VACUUM_REDUCED_MEMORY;
}
