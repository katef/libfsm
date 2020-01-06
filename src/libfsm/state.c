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
	assert(fsm != NULL);

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
			return 0;
		}

		fsm->statealloc = n;
		fsm->states = tmp;
	}

	if (state != NULL) {
		*state = fsm->statecount;
	}

	{
		struct fsm_state *new;

		new = &fsm->states[fsm->statecount];

		new->end      = 0;
		new->visited  = 0;
		new->opaque   = NULL;
		new->epsilons = NULL;
		new->edges    = NULL;
	}

	fsm->statecount++;

	return 1;
}

int
fsm_addstate_bulk(struct fsm *fsm, size_t n)
{
	size_t i;

	assert(fsm != NULL);

	if (fsm->statecount + n <= fsm->statealloc) {
		for (i = 0; i < n; i++) {
			struct fsm_state *new;

			new = &fsm->states[fsm->statecount + i];

			new->end      = 0;
			new->visited  = 0;
			new->opaque   = NULL;
			new->epsilons = NULL;
			new->edges    = NULL;
		}

		fsm->statecount += n;

		return 1;
	}

/*
TODO: bulk add
if there's space already, just increment statecount
otherwise realloc to += twice as much more
*/

	for (i = 0; i < n; i++) {
		fsm_state_t dummy;

		if (!fsm_addstate(fsm, &dummy)) {
			return 0;
		}
	}

	return 1;
}

void
fsm_removestate(struct fsm *fsm, fsm_state_t state)
{
	fsm_state_t start, i;

	assert(fsm != NULL);
	assert(state < fsm->statecount);

	/* for endcount accounting */
	fsm_setend(fsm, state, 0);

	for (i = 0; i < fsm->statecount; i++) {
		state_set_remove(&fsm->states[i].epsilons, state);
		if (fsm->states[i].edges != NULL) {
			edge_set_remove_state(&fsm->states[i].edges, state);
		}
	}

	state_set_free(fsm->states[state].epsilons);
	edge_set_free(fsm->states[state].edges);

	if (fsm_getstart(fsm, &start) && start == state) {
		fsm_clearstart(fsm);
	}

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
			state_set_replace(&fsm->states[i].epsilons, fsm->statecount - 1, state);
			edge_set_replace_state(&fsm->states[i].edges, fsm->statecount - 1, state);
		}
	}

	fsm->statecount--;
}

