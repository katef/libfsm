/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "internal.h"

int
symbol_closure_bulk(const struct fsm *fsm, const struct state_set *set,
	struct state_set *closure[static FSM_SIGMA_COUNT])
{
	struct state_iter it;
	fsm_state_t s;
	int i;

	assert(fsm != NULL);
	assert(closure != NULL);

	for (i = 0; i <= FSM_SIGMA_MAX; i++) {
		closure[i] = NULL;
	}

	for (state_set_reset((void *) set, &it); state_set_next(&it, &s); ) {
		struct fsm_edge *e;
		struct edge_iter jt;

		for (e = edge_set_first(fsm->states[s].edges, &jt); e != NULL; e = edge_set_next(&jt)) {
			if (!state_set_copy(&closure[e->symbol], fsm->opt->alloc, e->sl)) {
				return 0;
			}
		}
	}

	return 1;
}

