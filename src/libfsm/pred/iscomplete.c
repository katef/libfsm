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
	struct edge_iter it;
	unsigned n;

	assert(fsm != NULL);
	assert(state != NULL);

	n = 0;

	/* TODO: assert state is in fsm->sl */
	for (e = edge_set_first(state->edges, &it); e != NULL; e = edge_set_next(&it)) {
		/* epsilon transitions have no effect on completeness */
		if (e->symbol > UCHAR_MAX) {
			continue;
		}

		if (state_set_empty(e->sl)) {
			continue;
		}

		n++;
	}

	return n == UCHAR_MAX + 1;
}

