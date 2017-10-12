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

/* Constraints for walking edges for subtraction (A-B) */
enum { FSM_WALK2_EDGE_SUBTRACT  = FSM_WALK2_ONLYA | FSM_WALK2_BOTH };

/* Constraints for end states for subtraction (A-B).
 *
 * Notice that the edge constraint for subtraction is different from the
 * end state constraint.  With subtraction, you want to follow edges
 * that are either ONLYA or BOTH, but valid end states must be ONLYA.
 */
enum { FSM_WALK2_END_SUBTRACT  = FSM_WALK2_ONLYA };

struct fsm *
fsm_subtract(struct fsm *a, struct fsm *b)
{
	return fsm_walk2(a,b, FSM_WALK2_EDGE_SUBTRACT, FSM_WALK2_END_SUBTRACT);
}

