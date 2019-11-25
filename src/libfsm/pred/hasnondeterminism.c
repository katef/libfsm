/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <stdio.h>
#include <assert.h>
#include <stddef.h>
#include <limits.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include <print/esc.h>

#include <adt/bitmap.h>
#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "../internal.h"

static int
state_hasnondeterminism(const struct fsm *fsm, fsm_state_t state, struct bm *bm)
{
	const struct fsm_edge *e;
	struct edge_iter jt;

	assert(fsm != NULL);
	assert(state < fsm->statecount);

	for (e = edge_set_first(fsm->states[state]->edges, &jt); e != NULL; e = edge_set_next(&jt)) {
		size_t n;

		n = state_set_count(e->sl);

		if (n == 0) {
			continue;
		}

		if (n > 1 || (bm != NULL && bm_get(bm, e->symbol))) {
			return 1;
		}

		if (bm != NULL) {
			bm_set(bm, e->symbol);
		}
	}

	return 0;
}

int
fsm_hasnondeterminism(const struct fsm *fsm, fsm_state_t state)
{
	struct state_set *ec;
	struct state_iter it;
	struct bm bm;
	fsm_state_t s;

	assert(fsm != NULL);
	assert(state < fsm->statecount);

	if (!fsm_hasepsilons(fsm, state)) {
		return state_hasnondeterminism(fsm, state, NULL);
	}

	ec = state_set_create(fsm->opt->alloc);
	if (ec == NULL) {
		return -1;
	}

	if (!epsilon_closure(fsm, state, ec)) {
		state_set_free(ec);
		return -1;
	}

	bm_clear(&bm);

	for (state_set_reset(ec, &it); state_set_next(&it, &s); ) {
		if (state_hasnondeterminism(fsm, s, &bm)) {
			state_set_free(ec);
			return 1;
		}
	}

	state_set_free(ec);

	return 0;
}

