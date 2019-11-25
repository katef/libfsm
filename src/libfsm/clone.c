/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "internal.h"

struct fsm *
fsm_clone(const struct fsm *fsm)
{
	struct fsm *new;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	new = fsm_new(fsm->opt);
	if (new == NULL) {
		return NULL;
	}

	/*
	 * Create states corresponding to the origional FSM's states.
	 * These are created in reverse order, but that's okay.
	 */
	/* TODO: possibly centralise as a state-cloning function */
	{
		size_t i;

		/* TODO: malloc and memcpy in one shot, rather than
		 * calling fsm_addstate() for each state. */
		for (i = 0; i < fsm->statecount; i++) {
			fsm_state_t q;

			if (!fsm_addstate(new, &q)) {
				fsm_free(new);
				return NULL;
			}

			assert(q == i);
		}
	}

	{
		size_t i;

		for (i = 0; i < fsm->statecount; i++) {
			struct fsm_edge *e;
			struct edge_iter it;

			fsm_setend(new, i, fsm_isend(fsm, i));
			new->states[i]->opaque = fsm->states[i]->opaque;

			{
				struct state_iter jt;
				fsm_state_t to;

				for (state_set_reset(fsm->states[i]->epsilons, &jt); state_set_next(&jt, &to); ) {
					if (!fsm_addedge_epsilon(new, i, to)) {
						fsm_free(new);
						return NULL;
					}
				}
			}

			for (e = edge_set_first(fsm->states[i]->edges, &it); e != NULL; e = edge_set_next(&it)) {
				struct state_iter jt;
				fsm_state_t to;

				for (state_set_reset(e->sl, &jt); state_set_next(&jt, &to); ) {
					if (!fsm_addedge_literal(new, i, to, e->symbol)) {
						fsm_free(new);
						return NULL;
					}
				}
			}
		}
	}

	{
		fsm_state_t start;

		if (fsm_getstart(fsm, &start)) {
			fsm_setstart(new, start);
		}
	}

	fsm_clear_tmp(new);

	return new;
}

