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
	struct fsm_edge *e;
	struct set_iter it;
	size_t i;

	assert(fsm != NULL);
	assert(state != NULL);

	/* TODO: assert state is in fsm->sl */
	i = 0;
	for (e = set_first(state->edges, &it); e != NULL; e = set_next(&it)) {
		if (e->symbol > UCHAR_MAX) {
			break;
		}

		if (set_empty(e->sl)) {
			return 0;
		}

		i++;
	}

	return i == UCHAR_MAX;
}

