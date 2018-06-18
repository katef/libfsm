/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

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

	assert(fsm != NULL);
	assert(state != NULL);

	/* epsilon transitions have no effect on completeness */

	/* TODO: assert state is in fsm->sl */
	for (e = set_first(state->edges, &it); e != NULL; e = set_next(&it)) {
		if (e->symbol > UCHAR_MAX) {
			return 1;
		}

		if (set_empty(e->sl)) {
			return 0;
		}

		if (e->symbol == UCHAR_MAX) {
			return 1;
		}
	}

	return 0;
}

