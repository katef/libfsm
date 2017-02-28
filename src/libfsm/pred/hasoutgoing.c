/* $Id$ */

#include <assert.h>
#include <stddef.h>

#include <adt/set.h>

#include <fsm/pred.h>

#include "../internal.h"

int
fsm_hasoutgoing(const struct fsm *fsm, const struct fsm_state *state)
{
	struct fsm_edge *e;
	struct set_iter it;

	assert(fsm != NULL);
	assert(state != NULL);

	(void) fsm;
	for (e = set_first(state->edges, &it); e != NULL; e = set_next(&it)) {
		if (!set_empty(e->sl)) {
			return 1;
		}
	}

	return 0;
}

