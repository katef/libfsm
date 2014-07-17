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
	const struct state_set *e;
	int i;

	assert(fsm != NULL);
	assert(state != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			for (e = s->edges[i].sl; e != NULL; e = e->next) {
				if (e->state == state) {
					return 1;
				}
			}
		}
	}

	return 0;
}

