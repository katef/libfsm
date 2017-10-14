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
fsm_intersect(struct fsm *a, struct fsm *b)
{
	struct fsm *q;

	/*
	 * This is intersection implemented by walking sets of states through
	 * both FSM simultaneously, as described by Hopcroft, Motwani and Ullman
	 * (2001, 2nd ed.) 4.2, Closure under Intersection.
	 *
	 * This could also be done in terms of complements and union,
	 * by DeMorgan's theorem:
	 *
	 *     a \n b = ~(~a \u ~b)
	 */

	/*
	 * The intersection of two FSM consists of only those items which
	 * are present in _BOTH.
	 */
	q = fsm_walk2(a, b, FSM_WALK2_BOTH, FSM_WALK2_BOTH);
	if (q == NULL) {
		return NULL;
	}

	fsm_free(a);
	fsm_free(b);

	return q;
}

