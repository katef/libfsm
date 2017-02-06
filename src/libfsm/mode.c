/* $Id$ */

#include <assert.h>
#include <stddef.h>

#include <adt/set.h>

#include <fsm/fsm.h>

#include "internal.h"

struct fsm_state *
fsm_findmode(const struct fsm_state *state, unsigned int *freq)
{
	struct set_iter it;
	struct fsm_state *s;
	int i, j;

	struct {
		struct fsm_state *state;
		unsigned int freq;
	} mode;

	mode.state = NULL;
	mode.freq = 1;

	assert(state != NULL);

	for (i = 0; i <= UCHAR_MAX; i++) {
		for (s = set_first(state->edges[i].sl, &it); s != NULL; s = set_next(&it)) {
			unsigned int curr;

			assert(s != NULL);
			assert(!set_hasnext(&it));

			curr = 0;

			/* count the remaining edes which have the same target */
			for (j = i + 1; j <= UCHAR_MAX; j++) {
				curr += set_contains(state->edges[j].sl, s);
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

