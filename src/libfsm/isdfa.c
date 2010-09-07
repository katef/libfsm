/* $Id$ */

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <fsm/fsm.h>

#include "internal.h"

static int
isdfastate(const struct fsm_state *state)
{
	/*
	 * DFA may not have epsilon edges or duplicate states.
	 *
	 * Since this implementation encodes duplicate states as hanging off
	 * epsilon transitions, we need simply check for the presence of those
	 * to cover both situations.
	 */
	return state->el == NULL;
}

int
fsm_isdfa(const struct fsm *fsm)
{
	const struct state_set *s;

	assert(fsm != NULL);
	assert(fsm->start != NULL);

	if (fsm->sl == NULL) {
		return 0;
	}

	for (s = fsm->sl; s; s = s->next) {
		if (!isdfastate(s->state)) {
			return 0;
		}
	}

	return 1;
}

