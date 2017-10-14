/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <stdlib.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>

#include "walk2.h"

struct fsm *
fsm_subtract(struct fsm *a, struct fsm *b)
{
	struct fsm *q;

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

