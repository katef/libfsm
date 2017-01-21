/* $Id$ */

#include <assert.h>
#include <stddef.h>
#include <limits.h>

#include <adt/set.h>

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
		if (set_empty(state->edges[i].sl)) {
			return 0;
		}
	}

	return 1;
}

