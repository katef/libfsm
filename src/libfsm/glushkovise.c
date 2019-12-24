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

/* TODO: centralise as a predicate over a state set */
static int
hasend(const struct fsm *fsm, const struct state_set *set)
{
	struct state_iter it;
	fsm_state_t s;

	assert(fsm != NULL);

	for (state_set_reset((void *) set, &it); state_set_next(&it, &s); ) {
		if (fsm_isend(fsm, s)) {
			return 1;
		}
	}

	return 0;
}

int
fsm_glushkovise(struct fsm *nfa)
{
	struct state_set **eclosures;
	fsm_state_t s;

	assert(nfa != NULL);

	eclosures = epsilon_closure_bulk(nfa);
	if (eclosures == NULL) {
		return 0;
	}

	for (s = 0; s < nfa->statecount; s++) {
		struct state_set *sclosures[FSM_SIGMA_COUNT] = { NULL };
		struct state_iter kt;
		fsm_state_t es;
		struct fsm_edge *e;
		struct edge_iter jt;
		int i;

		if (nfa->states[s].epsilons == NULL) {
			continue;
		}

		for (state_set_reset(eclosures[s], &kt); state_set_next(&kt, &es); ) {
			/* we already have edges from s */
			if (es == s) {
				continue;
			}

			if (!symbol_closure_bulk(nfa, es, eclosures, sclosures)) {
				/* TODO: free stuff */
				goto error;
			}
		}

		state_set_free(nfa->states[s].epsilons);
		nfa->states[s].epsilons = NULL;

		/* TODO: bail out early if there are no edges to create? */

		if (nfa->states[s].edges == NULL) {
			nfa->states[s].edges = edge_set_create(nfa->opt->alloc, fsm_state_cmpedges);
			if (nfa->states[s].edges == NULL) {
				/* TODO: free stuff */
				goto error;
			}
		}

		for (e = edge_set_first(nfa->states[s].edges, &jt); e != NULL; e = edge_set_next(&jt)) {
			if (sclosures[e->symbol] == NULL) {
				continue;
			}

			if (!state_set_copy(&e->sl, nfa->opt->alloc, sclosures[e->symbol])) {
				/* TODO: free stuff */
				goto error;
			}

			state_set_free(sclosures[e->symbol]);
			sclosures[e->symbol] = NULL;
		}

		for (i = 0; i <= FSM_SIGMA_MAX; i++) {
			struct fsm_edge *new;

			if (sclosures[i] == NULL) {
				continue;
			}

			new = f_malloc(nfa->opt->alloc, sizeof *new);
			if (new == NULL) {
				/* TODO: free stuff */
				goto error;
			}

			new->symbol = i;
			new->sl = sclosures[i];

			/* ownership belongs to the newly-created edge now */
			sclosures[i] = NULL;

			if (!edge_set_add(nfa->states[s].edges, new)) {
				/* TODO: free stuff */
				goto error;
			}
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
			if (!hasend(nfa, eclosures[s])) {
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

