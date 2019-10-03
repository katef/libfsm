/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/options.h>

#include <adt/set.h>
#include <adt/stateset.h>

#include "internal.h"

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
 */
struct state_set *
epsilon_closure(const struct fsm *fsm, fsm_state_t state, struct state_set *closure)
{
	struct state_iter it;
	fsm_state_t s;

	assert(fsm != NULL);
	assert(state < fsm->statecount);
	assert(closure != NULL);

	/* Find if the given state is already in the closure */
	if (state_set_contains(closure, state)) {
		return closure;
	}

	if (!state_set_add(closure, state)) {
		return NULL;
	}

	/* Follow each epsilon transition */
	for (state_set_reset(fsm->states[state]->epsilons, &it); state_set_next(&it, &s); ) {
		if (epsilon_closure(fsm, s, closure) == NULL) {
			return NULL;
		}
	}

	return closure;
}

