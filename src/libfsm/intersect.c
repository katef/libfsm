/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <stdlib.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>

#include "walk2.h"

struct fsm;

/* Constraint for walking edges for intersection (A&B) */
enum { FSM_WALK2_EDGE_INTERSECT = FSM_WALK2_BOTH };

/* Constraint for end state for intersection (A&B) */
enum { FSM_WALK2_END_INTERSECT  = FSM_WALK2_BOTH };

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

	q = fsm_walk2(a, b, FSM_WALK2_EDGE_INTERSECT, FSM_WALK2_END_INTERSECT);
	if (q == NULL) {
		return NULL;
	}

	fsm_free(a);
	fsm_free(b);

	return q;
}

