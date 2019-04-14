/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

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
		struct fsm_state *s;

		for (s = fsm->sl; s != NULL; s = s->next) {
			struct fsm_state *q;

			q = fsm_addstate(new);
			if (q == NULL) {
				fsm_free(new);
				return NULL;
			}

			s->tmp.equiv = q;
		}
	}

	{
		struct fsm_state *s;

		for (s = fsm->sl; s != NULL; s = s->next) {
			struct fsm_edge *e;
			struct edge_iter it;

			assert(s->tmp.equiv != NULL);

			if (*fsm->tail == s) {
				new->tail = &s->tmp.equiv;
			}

			fsm_setend(new, s->tmp.equiv, fsm_isend(fsm, s));
			s->tmp.equiv->opaque = s->opaque;

			{
				struct state_iter jt;
				struct fsm_state *to;

				for (to = state_set_first(s->epsilons, &jt); to != NULL; to = state_set_next(&jt)) {
					if (!fsm_addedge_epsilon(new, s->tmp.equiv, to->tmp.equiv)) {
						fsm_free(new);
						return NULL;
					}
				}
			}

			for (e = edge_set_first(s->edges, &it); e != NULL; e = edge_set_next(&it)) {
				struct state_iter jt;
				struct fsm_state *to;

				for (to = state_set_first(e->sl, &jt); to != NULL; to = state_set_next(&jt)) {
					if (!fsm_addedge_literal(new, s->tmp.equiv, to->tmp.equiv, e->symbol)) {
						fsm_free(new);
						return NULL;
					}
				}
			}
		}
	}

	new->start = fsm->start->tmp.equiv;

	fsm_clear_tmp(new);

	return new;
}

