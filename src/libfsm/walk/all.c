/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/walk.h>

#include "../internal.h"

int
fsm_all(const struct fsm *fsm,
	int (*predicate)(const struct fsm *, const struct fsm_state *))
{
	size_t i;

	assert(fsm != NULL);
	assert(predicate != NULL);

	for (i = 0; i < fsm->statecount; i++) {
		if (!predicate(fsm, fsm->states[i])) {
			return 0;
		}
	}

	return 1;
}

