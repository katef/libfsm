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

unsigned
fsm_count(const struct fsm *fsm,
	int (*predicate)(const struct fsm *, const struct fsm_state *))
{
	unsigned n;
	size_t i;

	assert(fsm != NULL);
	assert(predicate != NULL);

	n = 0;

	for (i = 0; i < fsm->statecount; i++) {
		n += !!predicate(fsm, fsm->states[i]);
	}

	return n;
}

