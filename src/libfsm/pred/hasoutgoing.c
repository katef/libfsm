/* $Id$ */

#include <assert.h>
#include <stddef.h>

#include <adt/set.h>

#include <fsm/pred.h>

#include "../internal.h"

int
fsm_hasoutgoing(const struct fsm *fsm, const struct fsm_state *state)
{
	int i;

	assert(fsm != NULL);
	assert(state != NULL);

	(void) fsm;

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		if (!set_empty(state->edges[i].sl)) {
			return 1;
		}
	}

	return 0;
}

