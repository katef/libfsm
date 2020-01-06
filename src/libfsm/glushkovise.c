/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include <adt/alloc.h>
#include <adt/set.h>
#include <adt/edgeset.h>
#include <adt/stateset.h>

#include "internal.h"

int
fsm_glushkovise(struct fsm *nfa)
{
	struct state_set **eclosures;
	fsm_state_t s;

	assert(nfa != NULL);

	eclosures = epsilon_closure(nfa);
	if (eclosures == NULL) {
		return 0;
	}

	for (s = 0; s < nfa->statecount; s++) {
		struct state_set *sclosures[FSM_SIGMA_COUNT] = { NULL };
		struct state_iter kt;
		fsm_state_t es;
		int i;

		if (nfa->states[s].epsilons == NULL) {
			continue;
		}

		for (state_set_reset(eclosures[s], &kt); state_set_next(&kt, &es); ) {
			/* we already have edges from s */
			if (es == s) {
				continue;
			}

			if (!symbol_closure(nfa, es, eclosures, sclosures)) {
				/* TODO: free stuff */
				goto error;
			}
		}

		state_set_free(nfa->states[s].epsilons);
		nfa->states[s].epsilons = NULL;

		/* TODO: bail out early if there are no edges to create? */

		for (i = 0; i <= FSM_SIGMA_MAX; i++) {
			if (sclosures[i] == NULL) {
				continue;
			}

			if (!edge_set_add_state_set(&nfa->states[s].edges, nfa->opt->alloc, i, sclosures[i])) {
				/* TODO: free stuff */
				goto error;
			}

			/* XXX: we took a copy, but i would prefer to bulk transplant ownership instead */
			state_set_free(sclosures[i]);
		}

		/* all elements in sclosures[] have been freed or moved to their
		 * respective newly-created edge, so there's nothing to free here */

		/*
		 * The current NFA state is an end state if any of its associated
		 * epsilon-clousre states are end states.
		 *
		 * Since we're operating on states in-situ, we only need to find
		 * out if the current state isn't already marked as an end state.
		 *
		 * However in either case we still need to carry opaques, because
		 * the set of opaque values may differ.
		 */
		if (!fsm_isend(nfa, s)) {
			if (!state_set_has(nfa,  eclosures[s], fsm_isend)) {
				continue;
			}

			fsm_setend(nfa, s, 1);
		}

		/*
		 * Carry through opaque values, if present. This isn't anything to do
		 * with the NFA conversion; it's meaningful only to the caller.
		 *
		 * The closure may contain non-end states, but at least one state is
		 * known to have been an end state.
		 */
		fsm_carryopaque(nfa, eclosures[s], nfa, s);

	}

	closure_free(eclosures, nfa->statecount);

	return 1;

error:

	/* TODO: free stuff */

	return 0;
}

