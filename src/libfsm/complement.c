/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <limits.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/bool.h>

#include "internal.h"

int
fsm_complement(struct fsm *fsm)
{
	struct fsm_state *s;

	assert(fsm != NULL);

	if (!fsm_all(fsm, NULL, fsm_iscomplete)) {
		if (!fsm_complete(fsm, NULL, fsm_isany)) {
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
		fsm_setend(fsm, s, !fsm_isend(NULL, fsm, s));
	}

	if (!fsm_trim(fsm)) {
		return 0;
	}

	return 1;
}

