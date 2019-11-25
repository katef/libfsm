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
	int (*predicate)(const struct fsm *, fsm_state_t))
{
	fsm_state_t i;
	unsigned n;

	assert(fsm != NULL);
	assert(predicate != NULL);

	n = 0;

	for (i = 0; i < fsm->statecount; i++) {
		n += !!predicate(fsm, i);
	}

	return n;
}

