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
	const struct fsm_state *s;
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
		for (i = 0; i <= UCHAR_MAX; i++) {
			struct fsm_state *to;

			assert(s->edges[i].sl != NULL);
			assert(s->edges[i].sl->state != NULL);
			assert(s->edges[i].sl->next == NULL);

			to = s->edges[i].sl->state;

			if (fsm_isend(fsm, to)) {
				fsm_removeends(fsm, to);
			} else if (!fsm_addend(fsm, to, colour)) {
				/* TODO: deal with error */
				return 0;
			}
		}
	}

	return 1;
}

