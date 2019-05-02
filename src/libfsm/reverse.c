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
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/options.h>

#include "internal.h"

int
fsm_reverse(struct fsm *fsm)
{
	struct fsm *new;
	struct fsm_state *end;
	struct state_set *endset;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	new = fsm_new(fsm->opt);
	if (new == NULL) {
		return 0;
	}

	if (fsm->endcount == 0) {
		new->start = fsm_addstate(new);
		if (new->start == NULL) {
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
	end = NULL;

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
		size_t i;

		for (i = 0; i < fsm->statecount; i++) {
			struct fsm_state *p;

			p = fsm_addstate(new);
			if (p == NULL) {
				fsm_free(new);
				return 0;
			}

			fsm->states[i]->tmp.equiv = p;

			if (fsm->states[i] == fsm->start) {
				end = p;
				fsm_setend(new, end, 1);
			}

			if (fsm_isend(fsm, fsm->states[i])) {
				if (!state_set_add(endset, fsm->states[i])) {
					state_set_free(endset);
					fsm_free(new);
					return 0;
				}

				if (fsm->endcount == 1) {
					assert(new->start == NULL);
					new->start = p;
				}
			}
		}
	}

	fsm_carryopaque(fsm, endset, new, end);

	/* Create reversed edges */
	{
		size_t i;

		for (i = 0; i < fsm->statecount; i++) {
			struct fsm_state *to;
			struct fsm_state *se;
			struct edge_iter it;
			struct fsm_edge *e;

			to = fsm->states[i]->tmp.equiv;

			assert(to != NULL);

			{
				struct state_iter jt;

				for (se = state_set_first(fsm->states[i]->epsilons, &jt); se != NULL; se = state_set_next(&jt)) {
					assert(se->tmp.equiv != NULL);

					if (!fsm_addedge_epsilon(new, se->tmp.equiv, to)) {
						state_set_free(endset);
						fsm_free(new);
						return 0;
					}
				}
			}
			for (e = edge_set_first(fsm->states[i]->edges, &it); e != NULL; e = edge_set_next(&it)) {
				struct state_iter jt;

				for (se = state_set_first(e->sl, &jt); se != NULL; se = state_set_next(&jt)) {
					assert(se->tmp.equiv != NULL);

					if (!fsm_addedge_literal(new, se->tmp.equiv, to, e->symbol)) {
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
		assert(new->start != NULL);
	}

	/*
	 * Mark the new start state. If there's only one state, we can indicate it
	 * directly. Otherwise we will be starting from a group of states, linked
	 * together by epsilon transitions.
	 */
	if (fsm->endcount > 1) {
		struct fsm_state *s;
		struct state_iter it;

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
			s = NULL;
		} else {
			for (s = state_set_first(endset, &it); s != NULL; s = state_set_next(&it)) {
				if (!fsm_hasincoming(fsm, s)) {
					break;
				}
			}
		}

		if (s != NULL) {
			new->start = s->tmp.equiv;
			assert(new->start != NULL);
		} else {
			new->start = fsm_addstate(new);
			if (new->start == NULL) {
				state_set_free(endset);
				fsm_free(new);
				return 0;
			}
		}

		for (s = state_set_first(endset, &it); s != NULL; s = state_set_next(&it)) {
			struct fsm_state *state;

			state = s->tmp.equiv;
			assert(state != NULL);

			if (state == new->start) {
				continue;
			}

			if (!fsm_addedge_epsilon(new, new->start, state)) {
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

