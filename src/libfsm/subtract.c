/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>

#include "walk2.h"

struct fsm *
fsm_subtract_bywalk(struct fsm *a, struct fsm *b)
{
	return fsm_subtract(a,b);
}

struct fsm *
fsm_subtract(struct fsm *a, struct fsm *b)
{
	return fsm_walk2(a,b, FSM_WALK2_EDGE_SUBTRACT, FSM_WALK2_END_SUBTRACT);
}

