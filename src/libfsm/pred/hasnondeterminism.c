/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <stdio.h>
#include <assert.h>
#include <stddef.h>
#include <limits.h>

#include <print/esc.h>

#include <adt/bitmap.h>
#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include <fsm/pred.h>

#include "../internal.h"

static int
state_hasnondeterminism(const struct fsm_state *state, struct bm *bm)
{
	const struct fsm_edge *e;
	struct edge_iter jt;

	assert(state != NULL);

	for (e = edge_set_first(state->edges, &jt); e != NULL; e = edge_set_next(&jt)) {
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
fsm_hasnondeterminism(const struct fsm *fsm, const struct fsm_state *state)
{
	const struct fsm_state *s;
	struct state_set *ec;
	struct state_iter it;
	struct bm bm;

	assert(fsm != NULL);
	assert(state != NULL);

	(void) fsm;

	assert(state != NULL);

	if (!fsm_hasepsilons(fsm, state)) {
		return state_hasnondeterminism(state, NULL);
	}

	ec = state_set_create(fsm->opt->alloc);
	if (ec == NULL) {
		return -1;
	}

	if (!epsilon_closure(state, ec)) {
		state_set_free(ec);
		return -1;
	}

	bm_clear(&bm);

	for (s = state_set_first(ec, &it); s != NULL; s = state_set_next(&it)) {
		if (state_hasnondeterminism(s, &bm)) {
			state_set_free(ec);
			return 1;
		}
	}

	state_set_free(ec);

	return 0;
}

