/* $Id$ */

#include <assert.h>
#include <stdlib.h>

#include <adt/set.h>

#include <fsm/pred.h>

#include "internal.h"

int
fsm_isend(const struct fsm *fsm, const struct fsm_state *state)
{
	assert(fsm != NULL);
	assert(state != NULL);

	(void) fsm;

	return !!state->end;
}

int
fsm_hasend(const struct fsm *fsm)
{
	const struct fsm_state *s;

	assert(fsm != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		if (fsm_isend(fsm, s)) {
			return 1;
		}
	}

	return 0;
}

