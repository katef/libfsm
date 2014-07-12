/* $Id$ */

#include <assert.h>
#include <stddef.h>

#include <adt/set.h>

#include <fsm/fsm.h>

#include "internal.h"

struct fsm_state *
fsm_findmode(const struct fsm_state *state)
{
	struct state_set *s;
	int i, j;
	struct {
		struct fsm_state *state;
		unsigned int freq;
	} mode;

	mode.state = NULL;
	mode.freq = 1;

	assert(state != NULL);

	for (i = 0; i <= UCHAR_MAX; i++) {
		for (s = state->edges[i].sl; s != NULL; s = s->next) {
			unsigned int curr;

			assert(s->state != NULL);
			assert(s->next == NULL);

			curr = 0;

			/* count the remaining edes which have the same target */
			for (j = i + 1; j <= UCHAR_MAX; j++) {
				curr += set_contains(state->edges[j].sl, s->state);
			}

			if (curr > mode.freq) {
				mode.freq  = curr;
				mode.state = s->state;
			}
		}
	}

	return mode.state;
}

