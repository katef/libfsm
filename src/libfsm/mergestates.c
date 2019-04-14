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

#include "internal.h"

struct fsm_state *
fsm_mergestates(struct fsm *fsm, struct fsm_state *a, struct fsm_state *b)
{
	struct fsm_edge *e;
	struct fsm_state *s;
	struct edge_iter it;

	/* edges from b */
	{
		struct state_iter jt;

		for (s = state_set_first(b->epsilons, &jt); s != NULL; s = state_set_next(&jt)) {
			if (!fsm_addedge_epsilon(fsm, a, s)) {
				return NULL;
			}
		}
	}
	for (e = edge_set_first(b->edges, &it); e != NULL; e = edge_set_next(&it)) {
		struct state_iter jt;

		for (s = state_set_first(e->sl, &jt); s != NULL; s = state_set_next(&jt)) {
			if (!fsm_addedge_literal(fsm, a, s, e->symbol)) {
				return NULL;
			}
		}
	}

	/* edges to b */
	for (s = fsm->sl; s != NULL; s = s->next) {
		if (state_set_contains(s->epsilons, b)) {
			state_set_remove(s->epsilons, b);

			if (!fsm_addedge_epsilon(fsm, s, a)) {
				return NULL;
			}
		}

		for (e = edge_set_first(s->edges, &it); e != NULL; e = edge_set_next(&it)) {
			state_set_remove(e->sl, b);

			if (state_set_contains(e->sl, b)) {
				if (!fsm_addedge_literal(fsm, s, a, e->symbol)) {
					return NULL;
				}
			}
		}
	}

	fsm_removestate(fsm, b);

	return a;
}

