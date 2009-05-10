/* $Id$ */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/graph.h>

#include "internal.h"

int
fsm_minimize(struct fsm *fsm)
{
	int r;
	int hasend;

	assert(fsm != NULL);

	/*
	 * This is a special case to account for FSMs with no end state; end states
	 * become start states during reversal. If no end state is present, a start
	 * state is neccessarily invented. That'll become an end state again after
	 * the second reversal, and then merged out to all states during conversion
	 * to a DFA.
	 *
	 * The net effect of that is that for an FSM with no end states, every
	 * state would be marked an end state after minimization. Here the absence
	 * of an end state is recorded, so that those markings may be removed.
	 */
	hasend = fsm_hasend(fsm);

	/*
	 * Brzozowski's algorithm.
	 */
	{
		r = fsm_reverse(fsm);
		if (r <= 0) {
			return r;
		}

		r = fsm_todfa(fsm);
		if (r <= 0) {
			return r;
		}

		r = fsm_reverse(fsm);
		if (r <= 0) {
			return r;
		}

		r = fsm_todfa(fsm);
		if (r <= 0) {
			return r;
		}
	}

	if (!hasend) {
		struct state_list *s;

		for (s = fsm->sl; s; s = s->next) {
			fsm_setend(fsm, &s->state, 0);
		}
	}

	return 1;
}

