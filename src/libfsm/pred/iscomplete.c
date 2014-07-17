/* $Id$ */

#include <assert.h>
#include <stddef.h>
#include <limits.h>

#include <fsm/pred.h>

#include "../internal.h"

int
fsm_iscomplete(const struct fsm *fsm, const struct fsm_state *state)
{
	size_t i;

	assert(fsm != NULL);
	assert(state != NULL);

	/* TODO: assert state is in fsm->sl */

	for (i = 0; i <= UCHAR_MAX; i++) {
		if (state->edges[i].sl == NULL) {
			return 0;
		}
	}

	return 1;
}

