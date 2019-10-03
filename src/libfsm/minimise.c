/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <adt/set.h>

#include "internal.h"

int
fsm_minimise(struct fsm *fsm)
{
	int r;

	assert(fsm != NULL);

	/*
	 * Brzozowski's algorithm.
	 */

	r = fsm_reverse(fsm);
	if (!r) {
		return 0;
	}

	r = fsm_determinise(fsm);
	if (!r) {
		return 0;
	}

	r = fsm_reverse(fsm);
	if (!r) {
		return 0;
	}

	r = fsm_determinise(fsm);
	if (!r) {
		return 0;
	}

	return 1;
}

