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
#include <fsm/out.h>
#include <fsm/options.h>

#include "internal.h"

int
fsm_reverse(struct fsm *fsm)
{
	struct fsm_state *end;
	struct set *endset;

	unsigned long endcount;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	/*
	 * The new end state is the previous start state. Because there is (at most)
	 * one start state, the new FSM will have at most one end state.
	 */
	end = fsm->start;

	/*
	 * The set of the previous end states.
	 *
	 * These carry through their opaque values to the new end state.
	 * This isn't anything to do with the reversing; it's meaningful
	 * only to the caller.
	 */
	endset = NULL;

	endcount = fsm->endcount;

	/*
	 * Perform the actual reversal by listing the reversed edges we need,
	 * destroying all current edges, then adding those.
	 */
	{
		struct fsm_state *s;
		struct set_iter it;

		struct {
			struct fsm_state *from;
			struct fsm_state *to;
			enum fsm_edge_type symbol;
		} *q = NULL;
		size_t lq = 0;
		size_t nq = 0;

		size_t i;

		/*
		 * Populate our list of reversed edges. This loop will leave the FSM
		 * without any edges at all, but otherwise consistent.
		 */
		for (s = fsm->sl; s != NULL; s = s->next) {
			struct fsm_edge *e;
			for (e = set_first(s->edges, &it); e != NULL; e = set_next(&it)) {
				struct fsm_state *to;
				struct set_iter jt;
				for (to = set_first(e->sl, &jt); to != NULL; to = set_next(&jt)) {
					if (lq <= nq) {
						void *tmp;
						lq = lq > 0 ? lq * 2 : 1024;
						tmp = realloc(q, lq * sizeof *q);
						if (!tmp) {
							free(q);
							set_free(endset);
							return 0;
						}
						q = tmp;
					}
					q[nq].from = s;
					q[nq].to = to;
					q[nq].symbol = e->symbol;
					nq++;
				}
				set_free(e->sl);
				free(e);
			}
			set_clear(s->edges);

			if (fsm_isend(fsm, s)) {
				if (!set_add(&endset, s)) {
					set_free(endset);
					return 0;
				}

				if (endcount == 1) {
					fsm->start = s;
				}
			}

			fsm_setend(fsm, s, s == end);
		}

		/* the FSM now has no edges; add a reversed edge for each one recorded */
		for (i = 0; i < nq; i++) {
			fsm_addedge(q[i].to, q[i].from, q[i].symbol);
		}

		free(q);
	}

	if (end != NULL && fsm->opt->carryopaque != NULL) {
		fsm->opt->carryopaque(endset, fsm, end);
	}

	/*
	 * Mark the new start state. If there's only one state, we can indicate it
	 * directly. Otherwise we will be starting from a group of states, linked
	 * together by epsilon transitions.
	 */
	{
		struct fsm_state *s;
		struct set_iter it;


		switch (endcount) {
		case 1:
			/* already handled above */
			assert(fsm->start != NULL);
			break;

		case 0:
			/*
			 * Transition to all states. I don't like this at all, but it is
			 * required to be able to minimise automata with no end states.
			 */

			fsm->start = fsm_addstate(fsm);
			if (fsm->start == NULL) {
				set_free(endset);
				return 0;
			}

			for (s = fsm->sl; s != NULL; s = s->next) {
				if (s == fsm->start) {
					continue;
				}

				if (!fsm_addedge_epsilon(fsm, fsm->start, s)) {
					set_free(endset);
					return 0;
				}
			}

			break;

		default:
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

			if (!fsm->opt->tidy) {
				s = NULL;
			} else {
				for (s = set_first(endset, &it); s != NULL; s = set_next(&it)) {
					if (!fsm_hasincoming(fsm, s)) {
						break;
					}
				}
			}

			if (s != NULL) {
				fsm->start = s;
				assert(fsm->start != NULL);
			} else {
				fsm->start = fsm_addstate(fsm);
				if (fsm->start == NULL) {
					set_free(endset);
					return 0;
				}
			}

			for (s = set_first(endset, &it); s != NULL; s = set_next(&it)) {
				if (s == fsm->start) {
					continue;
				}

				if (!fsm_addedge_epsilon(fsm, fsm->start, s)) {
					set_free(endset);
					return 0;
				}
			}

			break;
		}
	}

	set_free(endset);

	return 1;
}
