/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <fsm/fsm.h>

#include <adt/alloc.h>
#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "internal.h"

int
fsm_addstate(struct fsm *fsm, fsm_state_t *state)
{
	struct fsm_state *new;

	assert(fsm != NULL);

	new = f_malloc(fsm->opt->alloc, sizeof *new);
	if (new == NULL) {
		return 0;
	}

	new->end = 0;
	new->opaque = NULL;

	/*
	 * Sets for epsilon and labelled transitions are kept NULL
	 * until populated; this suits the most nodes in the bodies of
	 * typical FSM that do not have epsilons, and (less often)
	 * nodes that have no edges.
	 */
	new->epsilons = NULL;
	new->edges    = NULL;

	fsm_state_clear_tmp(new);

	if (fsm->statecount == (fsm_state_t) -1) {
		errno = ENOMEM;
		return 0;
	}

	/* TODO: something better than one contigious realloc */
	if (fsm->statecount == fsm->statealloc) {
		const size_t factor = 2; /* a guess */
		const size_t n = fsm->statealloc * factor;
		void *tmp;

		tmp = f_realloc(fsm->opt->alloc, fsm->states, n * sizeof *fsm->states);
		if (tmp == NULL) {
			f_free(fsm->opt->alloc, new);
			return 0;
		}

		fsm->statealloc = n;
		fsm->states = tmp;
	}

	if (state != NULL) {
		*state = fsm->statecount;
	}

	fsm->states[fsm->statecount] = new;
	fsm->statecount++;

	return 1;
}

void
fsm_state_clear_tmp(struct fsm_state *state)
{
	memset(&state->tmp, 0x00, sizeof(state->tmp));
}

void
fsm_removestate(struct fsm *fsm, fsm_state_t state)
{
	struct fsm_edge *e;
	struct edge_iter it;
	fsm_state_t start, i;

	assert(fsm != NULL);
	assert(state < fsm->statecount);

	/* for endcount accounting */
	fsm_setend(fsm, state, 0);

	for (i = 0; i < fsm->statecount; i++) {
		state_set_remove(fsm->states[i]->epsilons, state);
		for (e = edge_set_first(fsm->states[i]->edges, &it); e != NULL; e = edge_set_next(&it)) {
			state_set_remove(e->sl, state);
		}
	}

	for (e = edge_set_first(fsm->states[state]->edges, &it); e != NULL; e = edge_set_next(&it)) {
		state_set_free(e->sl);
		f_free(fsm->opt->alloc, e);
	}
	state_set_free(fsm->states[state]->epsilons);
	edge_set_free(fsm->states[state]->edges);

	if (fsm_getstart(fsm, &start) && start == state) {
		fsm_clearstart(fsm);
	}

	f_free(fsm->opt->alloc, fsm->states[state]);

	assert(fsm->statecount >= 1);

	/*
	 * In order to avoid a hole, we move the last state over the removed index.
	 * This need a pass over all transitions to renumber any which point to
	 * that (now rehoused) state.
	 * TODO: this is terribly expensive. I would rather a mechanism for keeping
	 * a hole, perhaps done by ranges over a globally-shared state array.
	 */
	if (state < fsm->statecount - 1) {
		fsm->states[state] = fsm->states[fsm->statecount - 1];

		for (i = 0; i < fsm->statecount - 1; i++) {
			state_set_replace(fsm->states[i]->epsilons, fsm->statecount - 1, state);
			for (e = edge_set_first(fsm->states[i]->edges, &it); e != NULL; e = edge_set_next(&it)) {
				state_set_replace(e->sl, fsm->statecount - 1, state);
			}
		}
	}

	fsm->statecount--;
}

