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

#include "internal.h"

struct fsm_state *
fsm_findmode(const struct fsm_state *state, unsigned int *freq)
{
	struct fsm_edge *e;
	struct edge_iter it;
	struct fsm_state *s;

	struct {
		struct fsm_state *state;
		unsigned int freq;
	} mode;

	mode.state = NULL;
	mode.freq = 1;

	assert(state != NULL);

	for (e = edge_set_first(state->edges, &it); e != NULL; e = edge_set_next(&it)) {
		struct state_iter jt;

		for (s = state_set_first(e->sl, &jt); s != NULL; s = state_set_next(&jt)) {
			struct edge_iter kt = it;
			struct fsm_edge *c;
			unsigned int curr;

			curr = 1; /* including ourself */

			/* Count the remaining edes which have the same target.
			 * This works because the edges are still sorted by
			 * symbol, so we don't have to walk the whole thing.
			 */
			for (c = edge_set_next(&kt); c != NULL; c = edge_set_next(&kt)) {
				if (state_set_contains(c->sl, s)) {
					curr++;
				}
			}

			if (curr > mode.freq) {
				mode.freq  = curr;
				mode.state = s;
			}
		}
	}

	if (freq != NULL) {
		*freq = mode.freq;
	}

	return mode.state;
}

