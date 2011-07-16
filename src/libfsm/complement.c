/* $Id$ */

#include <assert.h>
#include <limits.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/graph.h>

#include "internal.h"
#include "set.h"

int
fsm_complement(struct fsm *fsm)
{
	struct fsm_state *s;

	assert(fsm != NULL);
	assert(fsm_isdfa(fsm));
	assert(fsm_iscomplete(fsm));

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

