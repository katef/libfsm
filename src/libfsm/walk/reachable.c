/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <adt/set.h>
#include <adt/dlist.h>

#include <fsm/fsm.h>

#include "../internal.h"

static int
fsm_reachable(const struct fsm *fsm, const struct fsm_state *state,
	int any,
	int (*predicate)(const struct fsm *, const struct fsm_state *))
{
	struct dlist *list;
	struct dlist *p;

	assert(state != NULL);
	assert(predicate != NULL);

	/*
	 * Iterative depth-first search.
	 */
	/* TODO: write in terms of fsm_walk or some common iteration callback */

	list = NULL;

	if (!dlist_push(&list, (struct fsm_state *) state)) {
		return -1;
	}

	while (p = dlist_nextnotdone(list), p != NULL) {
		struct fsm_edge *e;
		struct set_iter it;

		if (any) {
			if (predicate(fsm, p->state)) {
				dlist_free(p);
				return 1;
			}
		} else {
			if (!predicate(fsm, p->state)) {
				dlist_free(p);
				return 0;
			}
		}

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

	dlist_free(list);

	if (any) {
		return 0;
	} else {
		return 1;
	}
}

int
fsm_reachableall(const struct fsm *fsm, const struct fsm_state *state,
	int (*predicate)(const struct fsm *, const struct fsm_state *))
{
	return fsm_reachable(fsm, state, 0, predicate);
}

int
fsm_reachableany(const struct fsm *fsm, const struct fsm_state *state,
	int (*predicate)(const struct fsm *, const struct fsm_state *))
{
	return fsm_reachable(fsm, state, 1, predicate);
}

