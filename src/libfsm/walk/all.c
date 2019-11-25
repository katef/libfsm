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
	int (*predicate)(const struct fsm *, fsm_state_t))
{
	fsm_state_t i;

	assert(fsm != NULL);
	assert(predicate != NULL);

	for (i = 0; i < fsm->statecount; i++) {
		if (!predicate(fsm, i)) {
			return 0;
		}
	}

	return 1;
}

