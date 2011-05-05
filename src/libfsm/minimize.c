/* $Id$ */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include <fsm/fsm.h>
#include <fsm/graph.h>

#include "internal.h"
#include "set.h"

/* TODO: explain not true equivalence; only intended for use here */
static int equivalent(struct fsm_state *a, struct fsm_state *b) {
	int i;

	assert(a != NULL);
	assert(b != NULL);

	/* TODO: is this over-zealous? must we check set equivalence for end colours, too? */
	if (!!a->cl != !!b->cl) {
		return 0;
	}

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		if (!set_equal(a->edges[i].sl, b->edges[i].sl)) {
			return 0;
		}
	}

	/* TODO: anything else? colours? */

	return 1;
}

/* Return true if the edges after o contains state */
/* TODO: centralise */
static int contains(struct fsm_edge edges[], int o, struct fsm_state *state) {
	int i;

	assert(edges != NULL);
	assert(state != NULL);

	for (i = o; i <= FSM_EDGE_MAX; i++) {
		if (set_contains(state, edges[i].sl)) {
			return 1;
		}
	}

	return 0;
}

/* Count the number of distinct states to which a state transitions */
/* TODO: centralise? */
static unsigned int counttargets(struct fsm_state *state) {
	unsigned int count;
	int i;

	assert(state != NULL);

	count = 0;

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		if (state->edges[i].sl == NULL) {
			continue;
		}

		/* Distinct targets only */
		if (contains(state->edges, i + 1, state->edges[i].sl->state)) {
			continue;
		}

		/* Note that this assumes the state is a DFA state */
		assert(state->edges[i].sl->next == NULL);

		count++;
	}

	return count;
}

/* TODO: centralise */
static void removestate(struct fsm *fsm, struct fsm_state *state) {
	struct fsm_state **s;
	int i;

	assert(fsm != NULL);

	for (s = &fsm->sl; *s != NULL; s = &(*s)->next) {
		if (*s == state) {
			struct fsm_state *next;

			next = (*s)->next;

			/* TODO: centralise */
			for (i = 0; i <= FSM_EDGE_MAX; i++) {
				set_free((*s)->edges[i].sl);
			}

			free(*s);

			/* TODO: free colours */

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
		if (!r) {
			return 0;
		}

		r = fsm_todfa(fsm);
		if (!r) {
			return 0;
		}

		r = fsm_reverse(fsm);
		if (!r) {
			return 0;
		}

		r = fsm_todfa(fsm);
		if (!r) {
			return 0;
		}
	}

	if (!hasend) {
		struct fsm_state *s;
		struct colour_set *c;

		for (s = fsm->sl; s != NULL; s = s->next) {
			for (c = s->cl; c != NULL; c = c->next) {
				if (!fsm_addend(fsm, s, c->colour)) {
					return 0;
				}
			}
		}
	}

	/*
	 * If the start state is equivalent to another state then merge them.
	 * This is a special case due to neccessarily adding a state during
	 * reversal under some situations. It's a bit of a hack and I don't
	 * like it at all.
	 *
	 * This is special-case code; it can only possibly occur for the start
	 * state.
	 */
	if (fsm_isend(fsm, fsm->start) || counttargets(fsm->start) > 1) {
		struct fsm_state *s;
		struct fsm_state *next;

		for (s = fsm->sl; s != NULL; s = next) {
			next = s->next;

			if (s == fsm->start) {
				continue;
			}

			if (equivalent(fsm->start, s)) {
				removestate(fsm, fsm->start);

				/* TODO: merge colour sets? */

				fsm->start = s;

				break;
			}
		}
	}

	return 1;
}

