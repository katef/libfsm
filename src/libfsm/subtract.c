/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include "walk2.h"

struct fsm *
fsm_subtract(struct fsm *a, struct fsm *b)
{
	struct fsm *q;

	assert(a != NULL);
	assert(b != NULL);

	if (!fsm_all(a, fsm_isdfa)) {
		if (!fsm_determinise(a)) {
			return NULL;
		}
	}

	if (!fsm_all(b, fsm_isdfa)) {
		if (!fsm_determinise(b)) {
			return NULL;
		}
	}

	/*
	 * Subtraction a \ b is equivalent to: a ∩ ~b where ~b is complement.
	 * We provide the same result by fsm_walk2() to save on the overhead
	 * of intersection.
	 */

	/*
	 * Notice that the edge constraint for subtraction is different from the
	 * end state constraint. With subtraction, we want to follow edges
	 * that are either _ONLYA or _BOTH, but valid end states must be _ONLYA.
	 */
	q = fsm_walk2(a, b, FSM_WALK2_ONLYA | FSM_WALK2_BOTH, FSM_WALK2_ONLYA);
	if (q == NULL) {
		return NULL;
	}

	fsm_free(a);
	fsm_free(b);

	return q;
}

