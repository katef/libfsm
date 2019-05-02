/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <adt/alloc.h>
#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include <fsm/fsm.h>

#include "internal.h"

unsigned int
indexof(const struct fsm *fsm, const struct fsm_state *state)
{
	unsigned int i;

	assert(fsm != NULL);
	assert(state != NULL);

	for (i = 0; i < fsm->statecount; i++) {
		if (fsm->states[i] == state) {
			return i;
		}
	}

	assert(!"unreached");
	return 0;
}

struct fsm_state *
fsm_addstate(struct fsm *fsm)
{
	struct fsm_state *new;

	assert(fsm != NULL);

	new = f_malloc(fsm->opt->alloc, sizeof *new);
	if (new == NULL) {
		return NULL;
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

	/* TODO: something better than one contigious realloc */
	if (fsm->statecount == fsm->statealloc) {
		const size_t factor = 2; /* a guess */
		const size_t n = fsm->statealloc * factor;
		void *tmp;

		tmp = f_realloc(fsm->opt->alloc, fsm->states, n * sizeof *fsm->states);
		if (tmp == NULL) {
			f_free(fsm->opt->alloc, new);
			return NULL;
		}

		fsm->statealloc = n;
		fsm->states = tmp;
	}

	fsm->states[fsm->statecount] = new;
	fsm->statecount++;

	return new;
}

void
fsm_state_clear_tmp(struct fsm_state *state)
{
	memset(&state->tmp, 0x00, sizeof(state->tmp));
}

void
fsm_removestate(struct fsm *fsm, struct fsm_state *state)
{
	struct fsm_edge *e;
	struct edge_iter it;
	size_t i;

	assert(fsm != NULL);
	assert(state != NULL);

	/* for endcount accounting */
	fsm_setend(fsm, state, 0);

	for (i = 0; i < fsm->statecount; i++) {
		state_set_remove(fsm->states[i]->epsilons, state);
		for (e = edge_set_first(fsm->states[i]->edges, &it); e != NULL; e = edge_set_next(&it)) {
			state_set_remove(e->sl, state);
		}
	}

	for (e = edge_set_first(state->edges, &it); e != NULL; e = edge_set_next(&it)) {
		state_set_free(e->sl);
		f_free(fsm->opt->alloc, e);
	}
	state_set_free(state->epsilons);
	edge_set_free(state->edges);

	if (fsm->start == state) {
		fsm->start = NULL;
	}

	{
		size_t j;

		j = indexof(fsm, state);

		assert(fsm->statecount > 1);

		if (fsm->statecount - j > 1) {
			memmove(&fsm->states[j], &fsm->states[j + 1], sizeof *fsm->states * (fsm->statecount - j - 1));
		}
		fsm->statecount--;
	}

	f_free(fsm->opt->alloc, state);
}

