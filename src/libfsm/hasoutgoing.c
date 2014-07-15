/* $Id$ */

#include <assert.h>
#include <stddef.h>

#include <fsm/pred.h>

#include "internal.h"

int
fsm_hasoutgoing(const struct fsm *fsm, const struct fsm_state *state)
{
	int i;

	assert(fsm != NULL);
	assert(state != NULL);

	(void) fsm;

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		if (state->edges[i].sl != NULL) {
			return 1;
		}
	}

	return 0;
}

