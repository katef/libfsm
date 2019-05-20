/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <limits.h>
#include <stddef.h>

#include <adt/set.h>
#include <adt/dlist.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include "internal.h"

int
fsm_trim(struct fsm *fsm)
{
	struct dlist *list;
	struct dlist *p;
	struct fsm_state *start;

	/*
	 * Iterative depth-first search.
	 */
	/* TODO: write in terms of fsm_walk or some common iteration callback */

	list = NULL;

	start = fsm_getstart(fsm);
	if (start != NULL) {
		if (!dlist_push(fsm->opt->alloc, &list, start)) {
			return -1;
		}
	}

	while (p = dlist_nextnotdone(list), p != NULL) {
		struct fsm_edge *e;
		struct edge_iter it;

		{
			struct fsm_state *st;
			struct state_iter jt;

			for (st = state_set_first(p->state->epsilons, &jt); st != NULL; st = state_set_next(&jt)) {
				/* not a list operation... */
				if (dlist_contains(list, st)) {
					continue;
				}

				if (!dlist_push(fsm->opt->alloc, &list, st)) {
					return -1;
				}
			}
		}

		for (e = edge_set_first(p->state->edges, &it); e != NULL; e = edge_set_next(&it)) {
			struct fsm_state *st;
			struct state_iter jt;

			for (st = state_set_first(e->sl, &jt); st != NULL; st = state_set_next(&jt)) {
				/* not a list operation... */
				if (dlist_contains(list, st)) {
					continue;
				}

				if (!dlist_push(fsm->opt->alloc, &list, st)) {
					return -1;
				}
			}
		}

		p->done = 1;
	}

	/*
	 * Remove all states which aren't reachable.
	 * These are a disjoint subgraph.
	 */
	{
		struct fsm_state *s, *next;

		for (s = fsm->sl; s != NULL; s = next) {
			next = s->next;

			if (!dlist_contains(list, s)) {
				fsm_removestate(fsm, s);
			}
		}
	}

	dlist_free(fsm->opt->alloc, list);

	return 1;
}

