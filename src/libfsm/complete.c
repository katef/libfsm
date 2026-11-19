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
#include <fsm/walk.h>

#include <print/esc.h>

#include <adt/set.h>
#include <adt/bitmap.h>
#include <adt/edgeset.h>

#include "internal.h"

int
fsm_complete(struct fsm *fsm,
	int (*predicate)(const struct fsm *, fsm_state_t))
{
	fsm_state_t new, s;

	assert(fsm != NULL);
	assert(predicate != NULL);

	if (!fsm_all(fsm, fsm_isdfa)) {
		if (!fsm_determinise(fsm)) {
			fsm_free(fsm);
			return 0;
		}
	}

	if (!fsm_has(fsm, predicate)) {
		return 1;
	}

	/*
	 * A DFA is complete when every state has an edge for every symbol in the
	 * alphabet. For a typical low-density FSM, most of these will go to an
	 * "error" state, which is a non-end state that has every edge going back
	 * to itself. That error state is implicit in most FSMs, but rarely
	 * actually drawn. The idea here is to explicitly create it.
	 */
	if (!fsm_addstate(fsm, &new)) {
		return 0;
	}

	if (!fsm_addedge_any(fsm, new, new)) {
		/* TODO: free stuff */
		return 0;
	}

	for (s = 0; s < fsm->statecount; s++) {
		struct edge_group_iter it;
		struct edge_group_iter_info info;
		struct bm bm;

		if (!predicate(fsm, s)) {
			continue;
		}

		bm_clear(&bm);

		edge_set_group_iter_reset(fsm->states[s].edges, EDGE_GROUP_ITER_ALL, &it);
		while (edge_set_group_iter_next(&it, &info)) {
			for (size_t i = 0; i < 4; i++) {
				uint64_t *w = bm_nth_word(&bm, i);
				*w |= info.symbols[i];
			}
		}

		bm_invert(&bm);

		if (!bm_any(&bm)) {
			continue;
		}

		if (!edge_set_add_bulk(&fsm->states[s].edges, fsm->alloc, bm.map, new)) {
			/* TODO: free stuff */
			return 0;
		}
	}

	return 1;
}

