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
	struct fsm_state *s;
	unsigned int i;

	assert(fsm != NULL);
	assert(state != NULL);

	for (s = fsm->sl, i = 0; s != NULL; s = s->next, i++) {
		if (s == state) {
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

	*fsm->tail = new;
	new->next = NULL;
	fsm->tail  = &new->next;

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
	struct fsm_state *s;
	struct fsm_edge *e;
	struct edge_iter it;

	assert(fsm != NULL);
	assert(state != NULL);

	/* for endcount accounting */
	fsm_setend(fsm, state, 0);

	for (s = fsm->sl; s != NULL; s = s->next) {
		state_set_remove(s->epsilons, state);
		for (e = edge_set_first(s->edges, &it); e != NULL; e = edge_set_next(&it)) {
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
		struct fsm_state **p;
		struct fsm_state **tail;

		tail = &fsm->sl;

		for (p = &fsm->sl; *p != NULL; p = &(*p)->next) {
			if (*p == state) {
				struct fsm_state *next;

				if (fsm->tail == &(*p)->next) {
					fsm->tail = tail;
				}

				next = (*p)->next;
				f_free(fsm->opt->alloc, *p);
				*p = next;
				break;
			}

			tail = &(*p)->next;
		}
	}
}
