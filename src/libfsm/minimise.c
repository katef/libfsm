/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

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
	struct fsm_edge *ae, *be;
	struct set_iter it;

	assert(a != NULL);
	assert(b != NULL);

	if (set_count(a->edges) != set_count(b->edges)) {
		return 0;
	}

	for (ae = set_first(a->edges, &it); ae != NULL; ae = set_next(&it)) {
		if ((be = fsm_hasedge(b, ae->symbol)) == NULL) {
			return 0;
		}

		if (!set_equal(ae->sl, be->sl)) {
			return 0;
		}
	}

	/* TODO: anything else? */

	return 1;
}

/* Return true if the edges after o contains state */
/* TODO: centralise */
static int
contains(struct set *edges, int o, struct fsm_state *state)
{
	struct fsm_edge *e, search;
	struct set_iter it;

	assert(edges != NULL);
	assert(state != NULL);

	search.symbol = o;
	for (e = set_firstafter(edges, &it, &search); e != NULL; e = set_next(&it)) {
		if (set_contains(e->sl, state)) {
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
	struct fsm_edge *e;
	struct set_iter it;
	unsigned n;

	assert(state != NULL);

	n = 0;

	for (e = set_first(state->edges, &it); e != NULL; e = set_next(&it)) {
		struct fsm_state *s;

		if (set_empty(e->sl)) {
			continue;
		}

		/* Note that this assumes the state is a DFA state */
		s = set_only(e->sl);

		/* Distinct targets only */
		if (contains(state->edges, e->symbol, s)) {
			continue;
		}

		n++;
	}

	return n;
}

int
fsm_minimise(struct fsm *fsm)
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
	hasend = fsm_has(fsm, NULL, fsm_isend);

	/*
	 * Brzozowski's algorithm.
	 */
	{
		r = fsm_reverse(fsm);
		if (!r) {
			return 0;
		}

		r = fsm_determinise(fsm);
		if (!r) {
			return 0;
		}

		r = fsm_reverse(fsm);
		if (!r) {
			return 0;
		}

		r = fsm_determinise(fsm);
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
	if (fsm_isend(NULL, fsm, fsm->start) || counttargets(fsm->start) > 1) {
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

				fsm_removestate(fsm, fsm->start);

				fsm->start = s;

				break;
			}
		}
	}

	return 1;
}

