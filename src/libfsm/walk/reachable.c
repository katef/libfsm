/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>

#include <adt/set.h>
#include <adt/dlist.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "../internal.h"

static int
fsm_reachable(const struct fsm *fsm, fsm_state_t state,
	int any,
	int (*predicate)(const struct fsm *, fsm_state_t))
{
	int r, stopcond;

	fsm_state_t *q;
	size_t i, qtop;

	assert(state < fsm->statecount);
	assert(predicate != NULL);

	for (i=0; i < fsm->statecount; i++) {
		fsm->states[i].visited = 0;
	}

	q = f_malloc(fsm->opt->alloc, fsm->statecount * sizeof *q);
	if (q == NULL) {
		return -1;
	}

	/*
	 * Iterative depth-first search.
	 */
	/* TODO: write in terms of fsm_walk or some common iteration callback */

	q[0] = state;
	qtop = 1;
	fsm->states[state].visited = 1;

	stopcond = (any) ? 1 : 0;

	while (qtop > 0) {
		struct edge_iter it;
		struct fsm_edge e;
		fsm_state_t state;
		int pred_result;

		assert(qtop > 0);

		state = q[--qtop];

		pred_result = !!predicate(fsm, state);
		if (pred_result == stopcond) {
			r = stopcond;
			goto cleanup;
		}

		for (edge_set_reset(fsm->states[state].edges, &it); edge_set_next(&it, &e); ) {
			if (fsm->states[e.state].visited == 0) {
				assert(qtop < fsm->statecount);

				q[qtop++] = e.state;
				fsm->states[e.state].visited = 1;
			}
		}
	}

	r = !stopcond;
	goto cleanup;

cleanup:
	for (i=0; i < fsm->statecount; i++) {
		fsm->states[i].visited = 0;
	}

	f_free(fsm->opt->alloc, q);

	return r;
}

int
fsm_reachableall(const struct fsm *fsm, fsm_state_t state,
	int (*predicate)(const struct fsm *, fsm_state_t))
{
	return fsm_reachable(fsm, state, 0, predicate);
}

int
fsm_reachableany(const struct fsm *fsm, fsm_state_t state,
	int (*predicate)(const struct fsm *, fsm_state_t))
{
	return fsm_reachable(fsm, state, 1, predicate);
}

