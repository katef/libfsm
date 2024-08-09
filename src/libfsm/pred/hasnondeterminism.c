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

/*
 * Return a set of each state in the epsilon closure of the given state.
 * These are all the states reachable through epsilon transitions (that is,
 * without consuming any input by traversing a labelled edge), including the
 * given state itself.
 *
 * Intermediate states consisting entirely of epsilon transitions are
 * considered part of the closure.
 *
 * Returns closure on success, NULL on error.
 *
 * This function is here just to keep it out of the way of the general purpose
 * implementation of epsilon closure, which operates on all states in bulk.
 */
static struct state_set *
state_epsilon_closure(const struct fsm *fsm, fsm_state_t state,
	struct state_set **closure)
{
	struct state_iter it;
	fsm_state_t s;

	assert(fsm != NULL);
	assert(state < fsm->statecount);
	assert(closure != NULL);

	/* Find if the given state is already in the closure */
	if (state_set_contains(*closure, state)) {
		return *closure;
	}

	if (!state_set_add(closure, fsm->alloc, state)) {
		return NULL;
	}

	/* Follow each epsilon transition */
	for (state_set_reset(fsm->states[state].epsilons, &it); state_set_next(&it, &s); ) {
		if (state_epsilon_closure(fsm, s, closure) == NULL) {
			return NULL;
		}
	}

	return *closure;
}

int
state_hasnondeterminism(const struct fsm *fsm, fsm_state_t state, struct bm *bm)
{
	assert(fsm != NULL);
	assert(state < fsm->statecount);
	assert(bm != NULL);

	return edge_set_hasnondeterminism(fsm->states[state].edges, bm);
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

	bm_clear(&bm);

	if (!fsm_hasepsilons(fsm, state)) {
		return state_hasnondeterminism(fsm, state, &bm);
	}

	ec = NULL;

	if (!state_epsilon_closure(fsm, state, &ec)) {
		state_set_free(ec);
		return -1;
	}

	for (state_set_reset(ec, &it); state_set_next(&it, &s); ) {
		if (state_hasnondeterminism(fsm, s, &bm)) {
			state_set_free(ec);
			return 1;
		}
	}

	state_set_free(ec);

	return 0;
}

