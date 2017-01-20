/* $Id$ */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include "internal.h"

/* TODO: explain not true equivalence; only intended for use here */
static int
equivalent(struct fsm_state *a, struct fsm_state *b)
{
	int i;

	assert(a != NULL);
	assert(b != NULL);

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		if (!set_equal(a->edges[i].sl, b->edges[i].sl)) {
			return 0;
		}
	}

	/* TODO: anything else? */

	return 1;
}

/* Return true if the edges after o contains state */
/* TODO: centralise */
static int
contains(struct fsm_edge edges[], int o, struct fsm_state *state)
{
	int i;

	assert(edges != NULL);
	assert(state != NULL);

	for (i = o; i <= FSM_EDGE_MAX; i++) {
		if (set_contains(edges[i].sl, state)) {
			return 1;
		}
	}

	return 0;
}

/* Count the number of distinct states to which a state transitions */
/* TODO: centralise? */
static unsigned
counttargets(struct fsm_state *state)
{
	unsigned n;
	int i;

	assert(state != NULL);

	n = 0;

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		if (set_empty(state->edges[i].sl)) {
			continue;
		}

		/* Distinct targets only */
		if (contains(state->edges, i + 1, state->edges[i].sl->state)) {
			continue;
		}

		/* Note that this assumes the state is a DFA state */
		assert(state->edges[i].sl->next == NULL);

		n++;
	}

	return n;
}

/* TODO: centralise */
static void
removestate(struct fsm *fsm, struct fsm_state *state)
{
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

			*s = next;

			return;
		}
	}
}

int
fsm_minimise_opaque(struct fsm *fsm,
	void (*carryopaque)(struct state_set *, struct fsm *, struct fsm_state *))
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
	hasend = fsm_has(fsm, fsm_isend);

	/*
	 * Brzozowski's algorithm.
	 */
	{
		r = fsm_reverse_opaque(fsm, carryopaque);
		if (!r) {
			return 0;
		}

		r = fsm_determinise_opaque(fsm, carryopaque);
		if (!r) {
			return 0;
		}

		r = fsm_reverse_opaque(fsm, carryopaque);
		if (!r) {
			return 0;
		}

		r = fsm_determinise_opaque(fsm, carryopaque);
		if (!r) {
			return 0;
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
				/* XXX: I *think* there's no need to carryopaque() to s,
				 * since newly-added start states would never have an opaque. */

				removestate(fsm, fsm->start);

				fsm->start = s;

				break;
			}
		}
	}

	return 1;
}

int
fsm_minimise(struct fsm *fsm)
{
	return fsm_minimise_opaque(fsm, NULL);
}

