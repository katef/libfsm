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
	struct edge_group_iter it;
	struct edge_group_iter_info info;
	struct bm bm;

	assert(fsm != NULL);
	assert(state < fsm->statecount);

	/* epsilon transitions have no effect on completeness */
	(void) fsm->states[state].epsilons;

	bm_clear(&bm);

	edge_set_group_iter_reset(fsm->states[state].edges, EDGE_GROUP_ITER_ALL, &it);
	while (edge_set_group_iter_next(&it, &info)) {
		assert(info.to < fsm->statecount);

		for (size_t i = 0; i < 4; i++) {
			uint64_t *w = bm_nth_word(&bm, i);
			*w |= info.symbols[i];
		}
	}

	return bm_count(&bm) == FSM_SIGMA_COUNT;
}
