/* $Id$ */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include <fsm/fsm.h>
#include <fsm/graph.h>

#include "internal.h"
#include "set.h"

/* TODO: explain not true equivalence; only intended for use here */
/* TODO: note that this is asymetric; really it's if a is a subset of b */
/* TODO: i.e. if b goes to the same places that a goes to */
static int equivalent(struct fsm_state *a, struct fsm_state *b) {
	struct state_set *s;
	int i;

	assert(a != NULL);
	assert(b != NULL);

	if (a->end != b->end) {
		return 0;
	}

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		/* TODO: centralise subsetof()? */
		for (s = a->edges[i]; s; s = s->next) {
			if (!set_contains(s->state, b->edges[i])) {
				return 0;
			}
		}
	}

	/* TODO: anything else? opaques? */

	return 1;
}

static void remove(struct fsm *fsm, struct fsm_state *state) {
	struct fsm_state **s;
	int i;

	assert(fsm != NULL);

	for (s = &fsm->sl; *s; s = &(*s)->next) {
		if (*s == state) {
			struct fsm_state *next;

			next = (*s)->next;

			/* TODO: centralise */
			for (i = 0; i <= FSM_EDGE_MAX; i++) {
				set_free((*s)->edges[i]);
			}

			free(*s);

			/* TODO: free opaques */

			*s = next;

			return;
		}
	}
}

int
fsm_minimize(struct fsm *fsm)
{
	int r;
	int hasend;

	assert(fsm != NULL);

	/*
	 * This is a special case to account for FSMs with no end state; end states
	 * become start states during reversal. If no end state is present, a start
	 * state is neccessarily invented. That'll become an end state again after
	 * the second reversal, and then merged out to all states during conversion
	 * to a DFA.
	 *
	 * The net effect of that is that for an FSM with no end states, every
	 * state would be marked an end state after minimization. Here the absence
	 * of an end state is recorded, so that those markings may be removed.
	 */
	hasend = fsm_hasend(fsm);

	/*
	 * Brzozowski's algorithm.
	 */
	{
		r = fsm_reverse(fsm);
		if (r <= 0) {
			return r;
		}

		r = fsm_todfa(fsm);
		if (r <= 0) {
			return r;
		}

		r = fsm_reverse(fsm);
		if (r <= 0) {
			return r;
		}

		r = fsm_todfa(fsm);
		if (r <= 0) {
			return r;
		}
	}

	if (!hasend) {
		struct fsm_state *s;

		for (s = fsm->sl; s; s = s->next) {
			fsm_setend(fsm, s, 0);
		}
	}

	/*
	 * If the start state is equivalent to another state then merge them.
	 * This is a special case due to neccessarily adding a state during
	 * reversal under some situations. It's a bit of a hack and I don't
	 * like it at all.
	 *
	 * This is special-case code; it can only possibly occur for the start
	 * state, and there can only ever be one equivalent candidate for merging.
	 */
	if (fsm->start->end) {
		struct fsm_state *s;
		struct fsm_state *next;

		for (s = fsm->sl; s; s = next) {
			next = s->next;

			if (s == fsm->start) {
				continue;
			}

			if (!s->end) {
				continue;
			}

			if (equivalent(fsm->start, s)) {
				remove(fsm, fsm->start);

				/* TODO: merge opaques? */

				fsm->start = s;

				break;
			}
		}
	}

	return 1;
}

