/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include <print/esc.h>

#include <adt/set.h>
#include <adt/bitmap.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "../internal.h"

int
fsm_iscomplete(const struct fsm *fsm, fsm_state_t state)
{
	struct fsm_edge e;
	struct edge_iter it;
	struct bm bm;

	assert(fsm != NULL);
	assert(state < fsm->statecount);

	/* epsilon transitions have no effect on completeness */
	(void) fsm->states[state].epsilons;

	bm_clear(&bm);

	for (edge_set_reset(fsm->states[state].edges, &it); edge_set_next(&it, &e); ) {
		assert(e.state < fsm->statecount);

		bm_set(&bm, e.symbol);
	}

	return bm_count(&bm) == FSM_SIGMA_COUNT;
}

