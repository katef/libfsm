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
		if (!dlist_push(&list, start)) {
			return -1;
		}
	}

	while (p = dlist_nextnotdone(list), p != NULL) {
		struct fsm_edge *e;
		struct set_iter it;

		for (e = set_first(p->state->edges, &it); e != NULL; e = set_next(&it)) {
			struct fsm_state *st;
			struct set_iter jt;

			for (st = set_first(e->sl, &jt); st != NULL; st = set_next(&jt)) {
				/* not a list operation... */
				if (dlist_contains(list, st)) {
					continue;
				}

				if (!dlist_push(&list, st)) {
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

	dlist_free(list);

	/*
	 * Remove all states which have no reachable end state henceforth.
	 * These are a trailing suffix which will never accept.
	 *
	 * Note it doesn't matter which order in which these are removed;
	 * removing a state in the middle will disconnect the remainer of
	 * the suffix. Then visiting nodes in that newly disjoint subgraph
	 * will still be found to have no reachable end state, and so are
	 * also removed.
	 */
	{
		struct fsm_state *s, *next;

		for (s = fsm->sl; s != NULL; s = next) {
			next = s->next;

			if (fsm_isend(fsm, s)) {
				continue;
			}

			/* XXX: trim away the start state? I am unsure about this. */
			if (s == fsm_getstart(fsm)) {
				continue;
			}

			if (!fsm_reachableany(fsm, s, fsm_isend)) {
				fsm_removestate(fsm, s);
			}
		}
	}

	return 1;
}

