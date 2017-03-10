#include <assert.h>
#include <stddef.h>
#include <limits.h>
#include <errno.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include "internal.h"

int
fsm_empty(struct fsm *fsm)
{

	assert(fsm != NULL);

	if (!fsm_minimise(fsm)) {
		return -1;
	}

	/*
	 * After minimisation, all states are reachable.
	 * If any state is an end state, the FSM matches something.
	 * Note this includes the empty string (when the start state matches).
	 */

	if (fsm_has(fsm, fsm_isend)) {
		return 0;
	}

	return 1;
}

