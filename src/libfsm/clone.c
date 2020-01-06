/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "internal.h"

struct fsm *
fsm_clone(const struct fsm *fsm)
{
	struct fsm *new;
	size_t i;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	new = fsm_new(fsm->opt);
	if (new == NULL) {
		return NULL;
	}

	if (!fsm_addstate_bulk(new, fsm->statecount)) {
		fsm_free(new);
		return NULL;
	}

	for (i = 0; i < fsm->statecount; i++) {
		if (fsm_isend(fsm, i)) {
			fsm_setend(new, i, 1);
		}
		new->states[i].opaque = fsm->states[i].opaque;

		if (!state_set_copy(&new->states[i].epsilons, new->opt->alloc, fsm->states[i].epsilons)) {
			fsm_free(new);
			return NULL;
		}

		if (!edge_set_copy(&new->states[i].edges, new->opt->alloc, fsm->states[i].edges)) {
			fsm_free(new);
			return NULL;
		}
	}

	{
		fsm_state_t start;

		if (fsm_getstart(fsm, &start)) {
			fsm_setstart(new, start);
		}
	}

	return new;
}

