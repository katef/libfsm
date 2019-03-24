/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include <fsm/fsm.h>
#include <fsm/walk.h>

#include "../internal.h"

/* Allows iterating through the states of the graph with a callback
 * function.  Takes an opaque pointer that the callback can use for its
 * own purposes.
 *
 * If the callback returns 0, will stop iterating and return 0.
 * Otherwise will call the callback for each state and return 1.
 */
int
fsm_walk_states(const struct fsm *fsm, void *opaque,
	int (*callback)(const struct fsm *, const struct fsm_state *, void *))
{
	const struct fsm_state *s;

	assert(fsm != NULL);
	assert(callback != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		if (!callback(fsm, s, opaque)) {
			return 0;
		}
	}

	return 1;
}

/* Allows iterating through the states of the graph with a callback
 * function.  Takes an opaque pointer that the callback can use for its
 * own purposes.
 */
int
fsm_walk_edges(const struct fsm *fsm, void *opaque,
	int (*callback)(const struct fsm *, const struct fsm_state *, unsigned int, const struct fsm_state *, void *))
{
	const struct fsm_state *src;

	assert(fsm != NULL);
	assert(callback != NULL);

	for (src = fsm->sl; src != NULL; src = src->next) {
		struct edge_iter ei;
		const struct fsm_edge *e;

		for (e = edge_set_first(src->edges, &ei); e != NULL; e=edge_set_next(&ei)) {
			struct state_iter di;
			const struct fsm_state *dst;
			for (dst = state_set_first(e->sl, &di); dst != NULL; dst=state_set_next(&di)) {
				if (!callback(fsm, src, (unsigned int)e->symbol, dst, opaque)) {
					return 0;
				}
			}
		}
	}

	return 1;
}

