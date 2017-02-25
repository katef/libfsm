/* $Id$ */

#include <assert.h>
#include <stddef.h>

#include <adt/set.h>

#include <fsm/fsm.h>

#include "internal.h"

struct fsm_state *
fsm_findmode(const struct fsm_state *state, unsigned int *freq)
{
	struct fsm_edge *e;
	struct set_iter it;
	struct fsm_state *s;

	struct {
		struct fsm_state *state;
		unsigned int freq;
	} mode;

	mode.state = NULL;
	mode.freq = 1;

	assert(state != NULL);

	for (e = set_first(state->edges, &it); e != NULL; e = set_next(&it)) {
		struct set_iter jt;

		if (e->symbol > UCHAR_MAX) {
			break;
		}

		for (s = set_first(e->sl, &jt); s != NULL; s = set_next(&jt)) {
			struct set_iter kt = it;
			struct fsm_edge *c;
			unsigned int curr;

			assert(s != NULL);
			assert(!set_hasnext(&it));

			curr = 0;

			/* Count the remaining edes which have the same target.
			 * This works because the edges are still sorted by
			 * symbol, so we don't have to walk the whole thing.
			 */
			for (c = set_next(&kt); c != NULL; c = set_next(&kt)) {
				if (set_contains(c->sl, s)) {
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

