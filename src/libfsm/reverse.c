/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/options.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "internal.h"

int
fsm_reverse(struct fsm *fsm)
{
	struct state_set *endset;
	fsm_state_t prevstart;
	fsm_state_t start, end;
	int hasepsilons;
	struct edge_set **edges;
	struct state_set **epsilons;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	/*
	 * Reversing an FSM means to reverse the language the FSM matches.
	 * The textbook approach to this is to mark the start state as accepting,
	 * mark the accepting states as a new start state, and flip all the edges:
	 *
	 *    ⟶ ○ ⟶ ○ ⟶ ○ ⟶ ◎
	 *              ↺
	 *
	 * reversed:     
	 *
	 *      ◎ ← ○ ← ○ ← ○ ←
	 *              ↻
	 *
	 * However this doesn't explain what to do with a set of multiple accepting
	 * states (since an FSM may only have one start state). In general these
	 * can be unioned by epsilon transitions from a single newly-introduced
	 * start state. In this implementation we attempt to avoid introducing
	 * that new state where possible.
	 */

	if (fsm->endcount == 0 || !fsm_getstart(fsm, &prevstart)) {
		struct fsm *new;

		new = fsm_new(fsm->opt);
		if (new == NULL) {
			return 0;
		}

		if (!fsm_addstate(new, &new->start)) {
			fsm_free(new);
			return 0;
		}

		fsm_move(fsm, new);

		return 1;
	}

	/*
	 * The new end state is the previous start state. Because there is (at most)
	 * one start state, the new FSM will have at most one end state.
	 */
	end = prevstart;

	/*
	 * The set of the previous end states.
	 *
	 * These carry through their opaque values to the new end state.
	 * This isn't anything to do with the reversing; it's meaningful
	 * only to the caller.
	 */
	{
		fsm_state_t i;

		endset = NULL;

		for (i = 0; i < fsm->statecount; i++) {
			if (!fsm_isend(fsm, i)) {
				continue;
			}

			if (!state_set_add(&endset, fsm->opt->alloc, i)) {
				state_set_free(endset);
				return 0;
			}
		}

		assert(state_set_count(endset) == fsm->endcount);
	}

	/*
	 * The new start state is either the single previous end state, or
	 * if the endset contains multiple end states then the new start
	 * state is a specially-created state.
	 */
	{
		if (fsm->endcount == 1) {
			start = state_set_only(endset);
		} else {
			if (!fsm_addstate(fsm, &start)) {
				state_set_free(endset);
				return 0;
			}
		}

		fsm_setstart(fsm, start);
	}

	edges = f_malloc(fsm->opt->alloc, sizeof *edges * fsm->statecount);
	if (edges == NULL) {
		state_set_free(endset);
		return 0;
	}

	epsilons = f_malloc(fsm->opt->alloc, sizeof *epsilons * fsm->statecount);
	if (edges == NULL) {
		state_set_free(endset);
		f_free(fsm->opt->alloc, edges);
		return 0;
	}

	/*
	 * These arrays need to be initialised because we're populating the
	 * destination states here (that is, we set elements out of order).
	 */
	{
		fsm_state_t i;

		for (i = 0; i < fsm->statecount; i++) {
			epsilons[i] = NULL;
			edges[i]    = NULL;
		}
	}

	hasepsilons = 0;

	/* Create reversed edges */
	{
		fsm_state_t i;

		for (i = 0; i < fsm->statecount; i++) {
			struct edge_iter it;
			struct fsm_edge e;

			{
				struct state_iter jt;
				fsm_state_t se;

				/* TODO: eventually to be a predicate bit cached for the whole fsm */
				if (fsm->states[i].epsilons != NULL) {
					hasepsilons = 1;
				}

				for (state_set_reset(fsm->states[i].epsilons, &jt); state_set_next(&jt, &se); ) {
					if (!state_set_add(&epsilons[se], fsm->opt->alloc, i)) {
						goto error1;
					}
				}
			}
			for (edge_set_reset(fsm->states[i].edges, &it); edge_set_next(&it, &e); ) {
				assert(e.state < fsm->statecount);

				if (!edge_set_add(&edges[e.state], fsm->opt->alloc, e.symbol, i)) {
					return 0;
				}
			}
		}
	}

	/*
	 * Mark the new start state. If there's only one state, we can
	 * indicate it directly (done earlier). Otherwise we will be
	 * starting from a group of states, linked together by epsilon
	 * transitions (or an epsilon closure).
	 *
	 * We avoid introducing epsilon transitions to an FSM where
	 * there potentially were none before. That is, a Glushkov NFA
	 * will not be converted to a Thompson NFA.
	 *
	 * Conceptually this is equivalent to hooking the start state
	 * up with epsilons, then taking the epsilon closure of that
	 * state, and all outgoing transitions from that closure are
	 * copied over to the start state.
	 *
	 * If the FSM had epsilons in the first place, then there's no
	 * need to avoid introducing them here.
	 */
	if (fsm->endcount == 1) {
		/* we nominated the single previous end state as the new start */
	} else if (hasepsilons) {
		struct state_iter it;
		fsm_state_t s;

		for (state_set_reset(endset, &it); state_set_next(&it, &s); ) {
			if (s == start) {
				continue;
			}

			if (!state_set_add(&epsilons[start], fsm->opt->alloc, s)) {
				goto error1;
			}
		}
	} else {
		struct state_iter it;
		fsm_state_t s;

		for (state_set_reset(endset, &it); state_set_next(&it, &s); ) {
			if (s == start) {
				continue;
			}

			if (edges[s] == NULL) {
				continue;
			}

			if (!edge_set_copy(&edges[start], fsm->opt->alloc, edges[s])) {
				goto error;
			}
		}
	}

	{
		struct state_iter it;
		fsm_state_t s;

		if (!state_set_contains(endset, end)) {
			assert(!fsm_isend(fsm, end));

			fsm_setend(fsm, end, 1);

			/* TODO: if we keep a fsm-wide endset, we can use it verbatim here */
		}

		for (state_set_reset(endset, &it); state_set_next(&it, &s); ) {
			if (s == end) {
				continue;
			}

			fsm_setend(fsm, s, 0);
		}
	}

	/*
	 * If the epsilon closure of a newly-created start state includes an
	 * accepting state, then the start state itself must be marked accepting.
	 */
	if (state_set_count(endset) > 1 && !hasepsilons && state_set_has(fsm, endset, fsm_isend)) {
		assert(!fsm_isend(fsm, start));
		fsm_setend(fsm, start, 1);
	}

	{
		fsm_state_t i;

		for (i = 0; i < fsm->statecount; i++) {
			state_set_free(fsm->states[i].epsilons);
			edge_set_free(fsm->opt->alloc, fsm->states[i].edges);

			fsm->states[i].epsilons = epsilons[i];
			fsm->states[i].edges = edges[i];
		}
	}

	state_set_free(endset);

	f_free(fsm->opt->alloc, epsilons);
	f_free(fsm->opt->alloc, edges);

	return 1;

error1:

	{
		fsm_state_t i;

		for (i = 0; i < fsm->statecount; i++) {
			state_set_free(fsm->states[i].epsilons);
			edge_set_free(fsm->opt->alloc, fsm->states[i].edges);

			fsm->states[i].epsilons = epsilons[i];
			fsm->states[i].edges = edges[i];
		}
	}

error:

	state_set_free(endset);

	f_free(fsm->opt->alloc, epsilons);
	f_free(fsm->opt->alloc, edges);

	return 0;
}

