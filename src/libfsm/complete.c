/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <limits.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include "internal.h"

int
fsm_complete(struct fsm *fsm,
	int (*predicate)(const struct fsm *, const struct fsm_state *))
{
	struct fsm_state *new;
	struct fsm_state *s;

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
	new = fsm_addstate(fsm);
	if (new == NULL) {
		return 0;
	}

	if (!fsm_addedge_any(fsm, new, new)) {
		/* TODO: free stuff */
		return 0;
	}

	for (s = fsm->sl; s != NULL; s = s->next) {
		unsigned i;

		if (!predicate(fsm, s)) {
			continue;
		}

		for (i = 0; i <= UCHAR_MAX; i++) {
			if (fsm_hasedge_literal(s, i)) {
				continue;
			}

			if (!fsm_addedge_literal(fsm, s, new, i)) {
				/* TODO: free stuff */
				return 0;
			}
		}
	}

	return 1;
}

