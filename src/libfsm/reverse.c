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
	int hasepsilons;

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
	endset = NULL;

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

			assert(p == i);

			if (hasstart && i == start) {
				end = p;
				fsm_setend(new, end, 1);
			}

			if (fsm_isend(fsm, i)) {
				if (!state_set_add(&endset, fsm->opt->alloc, i)) {
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

		hasepsilons = 0;

		for (i = 0; i < fsm->statecount; i++) {
			struct edge_iter it;
			struct fsm_edge *e;
			fsm_state_t to;
			fsm_state_t se;

			to = i;

			assert(to < new->statecount);

			{
				struct state_iter jt;

				/* TODO: eventually to be a predicate bit cached for the whole fsm */
				if (fsm->states[i].epsilons != NULL) {
					hasepsilons = 1;
				}

				for (state_set_reset(fsm->states[i].epsilons, &jt); state_set_next(&jt, &se); ) {
					assert(se < fsm->statecount);
					assert(se < new->statecount);

					if (!fsm_addedge_epsilon(new, se, to)) {
						state_set_free(endset);
						fsm_free(new);
						return 0;
					}
				}
			}
			for (e = edge_set_first(fsm->states[i].edges, &it); e != NULL; e = edge_set_next(&it)) {
				struct state_iter jt;

				for (state_set_reset(e->sl, &jt); state_set_next(&jt, &se); ) {
					assert(se < fsm->statecount);
					assert(se < new->statecount);

					if (!fsm_addedge_literal(new, se, to, e->symbol)) {
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
	 * together by epsilon transitions (or an epsilon closure).
	 */
	if (fsm->endcount > 1) {
		struct state_iter it;
		fsm_state_t s, start;

		if (!fsm_addstate(new, &start)) {
			goto error;
		}

		fsm_setstart(new, start);

		/*
		 * Here we avoid introducing epsilon transitions to an FSM where
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
		if (hasepsilons) {
			for (state_set_reset(endset, &it); state_set_next(&it, &s); ) {
				if (s == start) {
					continue;
				}

				if (!fsm_addedge_epsilon(new, start, s)) {
					goto error;
				}
			}
		} else {
			if (new->states[start].edges == NULL) {
				new->states[start].edges = edge_set_create(fsm->opt->alloc, fsm_state_cmpedges);
				if (new->states[start].edges == NULL) {
					goto error;
				}
			}

			for (state_set_reset(endset, &it); state_set_next(&it, &s); ) {
				struct edge_iter jt;
				struct fsm_edge *e;

				if (s == start) {
					continue;
				}

				for (e = edge_set_first(new->states[s].edges, &jt); e != NULL; e = edge_set_next(&jt)) {
					struct fsm_edge *en;

					/* TODO: centralise with edge.c */

					en = edge_set_contains(new->states[start].edges, e);
					if (en == NULL) {
						en = f_malloc(new->opt->alloc, sizeof *en);
						if (en == NULL) {
							goto error;
						}

						en->symbol = e->symbol;
						en->sl = NULL;

						if (!edge_set_add(new->states[start].edges, en)) {
							goto error;
						}
					}

					if (!state_set_copy(&en->sl, new->opt->alloc, e->sl)) {
						goto error;
					}
				}

				if (!state_set_copy(&new->states[start].epsilons, new->opt->alloc, new->states[s].epsilons)) {
					goto error;
				}
			}

			/*
			 * If any of the endset states in the *new* fsm are accepting,
			 * then the new start state must also accept.
			 */
			if (!fsm_isend(new, start) && state_set_has(new, endset, fsm_isend)) {
				fsm_setend(new, start, 1);
				fsm_carryopaque(new, endset, new, start);
			}
		}
	}

	state_set_free(endset);

	fsm_move(fsm, new);

	return 1;

error:

	state_set_free(endset);
	fsm_free(new);

	return 0;
}

