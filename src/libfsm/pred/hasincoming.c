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

	assert(fsm != NULL);
	assert(state != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		struct fsm_edge *e;
		struct set_iter it;

		for (e = set_first(s->edges, &it); e != NULL; e = set_next(&it)) {
			if (set_contains(e->sl, state)) {
				return 1;
			}
		}
	}

	return 0;
}

