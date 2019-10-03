/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <limits.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <adt/set.h>

#include "internal.h"

int
fsm_empty(const struct fsm *fsm)
{
	fsm_state_t start;

	assert(fsm != NULL);

	if (!fsm_getstart(fsm, &start)) {
		errno = EINVAL;
		return -1;
	}

	/*
	 * If any reachable state is an end state, the FSM matches something.
	 * Note this includes the empty string (when the start state matches).
	 */

	if (fsm_reachableany(fsm, start, fsm_isend)) {
		return 0;
	}

	return 1;
}

