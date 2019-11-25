/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <limits.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <adt/set.h>

#include "internal.h"

int
fsm_complete(struct fsm *fsm,
	int (*predicate)(const struct fsm *, fsm_state_t))
{
	fsm_state_t new, i;

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

	for (i = 0; i < fsm->statecount; i++) {
		unsigned c;

		if (!predicate(fsm, i)) {
			continue;
		}

		for (c = 0; c <= UCHAR_MAX; c++) {
			if (fsm_hasedge_literal(fsm, i, c)) {
				continue;
			}

			if (!fsm_addedge_literal(fsm, i, new, c)) {
				/* TODO: free stuff */
				return 0;
			}
		}
	}

	return 1;
}

