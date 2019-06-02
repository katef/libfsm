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

int
fsm_walk_edges(const struct fsm *fsm, void *opaque,
	int (*callback_literal)(const struct fsm *, const struct fsm_state *, const struct fsm_state *, char c, void *),
	int (*callback_epsilon)(const struct fsm *, const struct fsm_state *, const struct fsm_state *, void *))
{
	const struct fsm_state *src;

	assert(fsm != NULL);
	assert(callback_literal != NULL);
	assert(callback_epsilon != NULL);

	for (src = fsm->sl; src != NULL; src = src->next) {
		const struct fsm_edge *e;
		struct edge_iter ei;

		for (e = edge_set_first(src->edges, &ei); e != NULL; e = edge_set_next(&ei)) {
			const struct fsm_state *dst;
			struct state_iter di;

			for (dst = state_set_first(e->sl, &di); dst != NULL; dst=state_set_next(&di)) {
				if (!callback_literal(fsm, src, dst, e->symbol, opaque)) {
					return 0;
				}
			}
		}

		{
			const struct fsm_state *dst;
			struct state_iter di;

			for (dst = state_set_first(src->epsilons, &di); dst != NULL; dst = state_set_next(&di)) {
				if (!callback_epsilon(fsm, src, dst, opaque)) {
					return 0;
				}
			}
		}
	}

	return 1;
}

