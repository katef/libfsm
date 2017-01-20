/* $Id$ */

#include <assert.h>
#include <stddef.h>

#include <adt/set.h>

#include <fsm/pred.h>

#include "../internal.h"

int
fsm_hasincoming(const struct fsm *fsm, const struct fsm_state *state)
{
	const struct fsm_state *s;
	int i;

	assert(fsm != NULL);
	assert(state != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			if (set_contains(s->edges[i].sl, state)) {
				return 1;
			}
		}
	}

	return 0;
}

