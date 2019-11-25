/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>

#include <adt/set.h>
#include <adt/dlist.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "../internal.h"

static int
fsm_reachable(const struct fsm *fsm, fsm_state_t state,
	int any,
	int (*predicate)(const struct fsm *, fsm_state_t))
{
	struct dlist *list;
	struct dlist *p;

	assert(state < fsm->statecount);
	assert(predicate != NULL);

	/*
	 * Iterative depth-first search.
	 */
	/* TODO: write in terms of fsm_walk or some common iteration callback */

	list = NULL;

	if (!dlist_push(fsm->opt->alloc, &list, state)) {
		return -1;
	}

	while (p = dlist_nextnotdone(list), p != NULL) {
		struct edge_iter it;
		struct fsm_edge *e;

		if (any) {
			if (predicate(fsm, p->state)) {
				dlist_free(fsm->opt->alloc, p);
				return 1;
			}
		} else {
			if (!predicate(fsm, p->state)) {
				dlist_free(fsm->opt->alloc, p);
				return 0;
			}
		}

		for (e = edge_set_first(fsm->states[p->state]->edges, &it); e != NULL; e = edge_set_next(&it)) {
			struct state_iter jt;
			fsm_state_t st;

			for (state_set_reset(e->sl, &jt); state_set_next(&jt, &st); ) {
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

	dlist_free(fsm->opt->alloc, list);

	if (any) {
		return 0;
	} else {
		return 1;
	}
}

int
fsm_reachableall(const struct fsm *fsm, fsm_state_t state,
	int (*predicate)(const struct fsm *, fsm_state_t))
{
	return fsm_reachable(fsm, state, 0, predicate);
}

int
fsm_reachableany(const struct fsm *fsm, fsm_state_t state,
	int (*predicate)(const struct fsm *, fsm_state_t))
{
	return fsm_reachable(fsm, state, 1, predicate);
}

