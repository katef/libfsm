/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <limits.h>

#include <fsm/fsm.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "internal.h"

int
fsm_addedge_epsilon(struct fsm *fsm,
	fsm_state_t from, fsm_state_t to)
{
	assert(fsm != NULL);
	assert(from < fsm->statecount);
	assert(to < fsm->statecount);

	if (!state_set_add(&fsm->states[from].epsilons, fsm->alloc, to)) {
		return 0;
	}

	return 1;
}

int
fsm_addedge_any(struct fsm *fsm,
	fsm_state_t from, fsm_state_t to)
{
	int i;

	assert(fsm != NULL);
	assert(from < fsm->statecount);
	assert(to < fsm->statecount);

	/* TODO: bulk add by symbol, presumably as a bitmap */
	for (i = 0; i <= UCHAR_MAX; i++) {
		if (!fsm_addedge_literal(fsm, from, to, (char)i)) {
			return 0;
		}
	}

	return 1;
}

int
fsm_addedge_literal(struct fsm *fsm,
	fsm_state_t from, fsm_state_t to,
	char c)
{
	assert(fsm != NULL);
	assert(from < fsm->statecount);
	assert(to < fsm->statecount);

	if (!edge_set_add(&fsm->states[from].edges, fsm->alloc, (unsigned char)c, to)) {
		return 0;
	}

	return 1;
}

