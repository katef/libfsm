/* $Id$ */

#include <assert.h>
#include <limits.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/graph.h>

#include "internal.h"
#include "set.h"
#include "colour.h"

int
fsm_complement(struct fsm *fsm, void *colour)
{
	struct fsm_state *s;
	int i;

	assert(fsm != NULL);
	assert(fsm_isdfa(fsm));
	assert(fsm_iscomplete(fsm));

	/*
	 * For a Complete DFA, the complement of the language matched is given by
	 * marking all the previous end states as non-end, and all the previous
	 * non-end states as end.
	 */

	for (s = fsm->sl; s != NULL; s = s->next) {
		if (fsm_isend(fsm, s)) {
			fsm_removeends(fsm, s);
		} else if (!fsm_addend(fsm, s, colour)) {
			/* TODO: deal with error */
			return 0;
		}
	}

	return 1;
}

