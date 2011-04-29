/* $Id$ */

#include <assert.h>

#include <fsm/fsm.h>

#include "internal.h"
#include "colour.h"
#include "set.h"

static int
ispure(const struct fsm *fsm, const struct fsm_state *state, void *colour, struct state_set **visited)
{
	struct fsm_state *s;
	int e;

	assert(fsm != NULL);
	assert(state != NULL);
	assert(visited != NULL);

	/*
	 * The "purity" of a state only cares about colour of edges. This is not to
	 * be confused with (if any) colours of states themselves.
	 *
	 * Purity ought to be determinable for an NFA as well as a DFA; I see no
	 * reason why not.
	 */

	if (!set_addstate(visited, state)) {
		/* TODO: error */
		return 0;
	}

	for (s = fsm->sl; s; s = s->next) {
		if (set_contains(s, *visited)) {
			continue;
		}

		for (e = 0; e <= FSM_EDGE_MAX; e++) {
			if (!set_contains(state, s->edges[e].sl)) {
				continue;
			}

			/* i.e. for each s[e] transitioning *to* state */

			/* if it's reachable by another colour (i think it==state?)
			 * then it must also be reachable by our colour */
			/* TODO: i.e. if there's at least one colour, and our colour is not also present: */
			if (!set_containscolour(fsm, colour, s->edges[e].cl)) {
				return 0;
			}

			if (!ispure(fsm, s, colour, visited)) {
				return 0;
			}
		}
	}

	return 1;
}

int
fsm_ispure(const struct fsm *fsm, const struct fsm_state *state, void *colour)
{
	struct state_set *visited;
	int r;

	assert(fsm != NULL);
	assert(state != NULL);

	/*
	 * This is another one of those typical lists which keep track of which
	 * states have been visited, to avoid infinite loops during graph traversal.
	 *
	 * It'd be much more sensible just to have a boolean which can be flagged
	 * within a state, that says if it's visited or not. That could be used for
	 * various different routines, including perhaps even the ungainly NFA to
	 * DFA conversion.
	 *
	 * For now I'm just using a list here (well, a set), because it keeps this
	 * interface self-contained, which is useful for sake of refactoring, while
	 * everything is still up in the air. Eventually all those similar "visited"
	 * lists could perhaps become bitmaps. And, potentially, when everything is
	 * settled, we can move those into the states themselves.
	 */
	visited = NULL;

	r = ispure(fsm, state, colour, &visited);

	set_free(visited);

	return r;
}

