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
	struct fsm *new;
	struct state_set *endset;
	fsm_state_t end;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	new = fsm_new(fsm->opt);
	if (new == NULL) {
		return 0;
	}

	if (fsm->endcount == 0) {
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
	(void) end;

	/*
	 * The set of the previous end states.
	 *
	 * These carry through their opaque values to the new end state.
	 * This isn't anything to do with the reversing; it's meaningful
	 * only to the caller.
	 */
	endset = state_set_create(fsm->opt->alloc);

	/*
	 * Create states corresponding to the original FSM's states.
	 * These are created in the same order, due to fsm_addstate()
	 * placing them at fsm->tail.
	 */
	/* TODO: possibly centralise as a state-copying function */
	{
		fsm_state_t start;
		int hasstart;
		size_t i;

		hasstart = fsm_getstart(fsm, &start);

		for (i = 0; i < fsm->statecount; i++) {
			fsm_state_t p;

			if (!fsm_addstate(new, &p)) {
				fsm_free(new);
				return 0;
			}

			fsm->states[i]->tmp.equiv = p;

			if (hasstart && i == start) {
				end = p;
				fsm_setend(new, end, 1);
			}

			if (fsm_isend(fsm, i)) {
				if (!state_set_add(endset, i)) {
					state_set_free(endset);
					fsm_free(new);
					return 0;
				}

				if (fsm->endcount == 1) {
					assert(!new->hasstart);
					fsm_setstart(new, p);
				}
			}
		}
	}

	fsm_carryopaque(fsm, endset, new, end);

	/* Create reversed edges */
	{
		fsm_state_t i;

		for (i = 0; i < fsm->statecount; i++) {
			struct edge_iter it;
			struct fsm_edge *e;
			fsm_state_t to;
			fsm_state_t se;

			to = fsm->states[i]->tmp.equiv;

			assert(to < new->statecount);

			{
				struct state_iter jt;

				for (state_set_reset(fsm->states[i]->epsilons, &jt); state_set_next(&jt, &se); ) {
					assert(fsm->states[se]->tmp.equiv < fsm->statecount);

					if (!fsm_addedge_epsilon(new, fsm->states[se]->tmp.equiv, to)) {
						state_set_free(endset);
						fsm_free(new);
						return 0;
					}
				}
			}
			for (e = edge_set_first(fsm->states[i]->edges, &it); e != NULL; e = edge_set_next(&it)) {
				struct state_iter jt;

				for (state_set_reset(e->sl, &jt); state_set_next(&jt, &se); ) {
					assert(fsm->states[se]->tmp.equiv < fsm->statecount);

					if (!fsm_addedge_literal(new, fsm->states[se]->tmp.equiv, to, e->symbol)) {
						state_set_free(endset);
						fsm_free(new);
						return 0;
					}
				}
			}
		}
	}

	assert(fsm->endcount != 0);

	if (fsm->endcount == 1) {
		/* already handled above */
		assert(new->start < fsm->statecount);
	}

	/*
	 * Mark the new start state. If there's only one state, we can indicate it
	 * directly. Otherwise we will be starting from a group of states, linked
	 * together by epsilon transitions.
	 */
	if (fsm->endcount > 1) {
		struct state_iter it;
		int have_start;
		fsm_state_t s, start;

		/*
		 * The typical case here is to create a new start state, and to
		 * link it to end states by epsilon transitions.
		 *
		 * An optimisation: if we found an end state with no incoming
		 * edges, that state is nominated as a new start state. This is
		 * equivalent to adding a new start state, and linking out from
		 * that, except it does not need to introduce a new state.
		 * Otherwise, we need to create a new start state as per usual.
		 *
		 * In either case, the start is linked to the other new start
		 * states by epsilons.
		 *
		 * It is important to use a start state with no incoming edges as
		 * this prevents accidentally transitioning to another route.
		 *
		 * This optimisation can be expensive to run, so it's optionally
		 * disabled by the opt->tidy flag.
		 */

		if (0 || !fsm->opt->tidy) {
			have_start = 0;
		} else {
			for (state_set_reset(endset, &it); state_set_next(&it, &s); ) {
				if (!fsm_hasincoming(fsm, s)) {
					have_start = 1;
					start = fsm->states[s]->tmp.equiv;
					break;
				}
			}
			have_start = 0;
		}

		if (have_start) {
			fsm_setstart(new, start);
		} else {
			if (!fsm_addstate(new, &start)) {
				state_set_free(endset);
				fsm_free(new);
				return 0;
			}

			fsm_setstart(new, start);
		}

		for (state_set_reset(endset, &it); state_set_next(&it, &s); ) {
			if (fsm->states[s]->tmp.equiv == start) {
				continue;
			}

			if (!fsm_addedge_epsilon(new, start, fsm->states[s]->tmp.equiv)) {
				state_set_free(endset);
				fsm_free(new);
				return 0;
			}
		}
	}

	state_set_free(endset);

	fsm_clear_tmp(fsm);

	fsm_move(fsm, new);

	return 1;
}

