/* $Id$ */

#include <assert.h>
#include <limits.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/graph.h>

#include "internal.h"

/* TODO: centralise */
static int
pred_all(const struct fsm *fsm, const struct fsm_state *state)
{
	assert(fsm != NULL);
	assert(state != NULL);

	(void) fsm;
	(void) state;

	return 1;
}

int
fsm_complement(struct fsm *fsm)
{
	struct fsm_state *s;

	assert(fsm != NULL);

	if (!fsm_iscomplete(fsm)) {
		if (!fsm_complete(fsm, pred_all)) {
			fsm_free(fsm);
			return 0;
		}
	}

	/*
	 * For a Complete DFA, the complement of the language matched is given by
	 * marking all the previous end states as non-end, and all the previous
	 * non-end states as end.
	 */

	for (s = fsm->sl; s != NULL; s = s->next) {
		fsm_setend(fsm, s, !fsm_isend(fsm, s));
	}

	return 1;
}

