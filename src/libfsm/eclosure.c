/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <adt/set.h>
#include <adt/stateset.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/options.h>

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
epsilon_closure(const struct fsm_state *state, struct state_set *closure)
{
	struct fsm_state *s;
	struct state_iter it;

	assert(state != NULL);
	assert(closure != NULL);

	/* Find if the given state is already in the closure */
	if (state_set_contains(closure, state)) {
		return closure;
	}

	if (!state_set_add(closure, (void *) state)) {
		return NULL;
	}

	/* Follow each epsilon transition */
	for (s = state_set_first(state->epsilons, &it); s != NULL; s = state_set_next(&it)) {
		assert(s != NULL);

		if (epsilon_closure(s, closure) == NULL) {
			return NULL;
		}
	}

	return closure;
}

