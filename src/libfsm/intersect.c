/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <stdlib.h>

#include "walk2.h"

struct fsm;

/* Constraint for walking edges for intersection (A&B) */
enum { FSM_WALK2_EDGE_INTERSECT = FSM_WALK2_BOTH };

/* Constraint for end state for intersection (A&B) */
enum { FSM_WALK2_END_INTERSECT  = FSM_WALK2_BOTH };

struct fsm *
fsm_intersect(struct fsm *a, struct fsm *b)
{
	return fsm_walk2(a,b, FSM_WALK2_EDGE_INTERSECT, FSM_WALK2_END_INTERSECT);
}

