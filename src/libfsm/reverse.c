/* $Id$ */

#include <assert.h>
#include <stddef.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include "internal.h"

/* TODO: centralise */
static struct fsm_state *
equivalent(const struct fsm *new, const struct fsm *fsm, const struct fsm_state *state)
{
	struct fsm_state *p;
	struct fsm_state *q;

	assert(new != NULL);
	assert(fsm != NULL);
	assert(state != NULL);

	for (p = fsm->sl, q = new->sl; p != NULL; p = p->next, q = q->next) {
		assert(q != NULL);

		if (p == state) {
			return q;
		}
	}

	return NULL;
}

int
fsm_reverse_opaque(struct fsm *fsm,
	void (*carryopaque)(struct set *, struct fsm *, struct fsm_state *))
{
	struct fsm *new;

	assert(fsm != NULL);

	new = fsm_new();
	if (new == NULL) {
		return 0;
	}

	/*
	 * Create states corresponding to the origional FSM's states.
	 * These are created in the same order, due to fsm_addstate()
	 * placing them at fsm->tail.
	 */
	/* TODO: possibly centralise as a state-copying function */
	{
		struct fsm_state *s;

		for (s = fsm->sl; s != NULL; s = s->next) {
			if (!fsm_addstate(new)) {
				fsm_free(new);
				return 0;
			}
		}
	}

	/*
	 * The new end state is the previous start state. Because there is (at most)
	 * one start state, the new FSM will have at most one end state.
	 */
	{
		struct fsm_state *end;

		end = equivalent(new, fsm, fsm->start);
		if (end != NULL) {
			fsm_setend(new, end, 1);
		}

		/*
		 * Carry through set of opaque values to the new end state.
		 * This isn't anything to do with the reversing; it's meaningful
		 * only to the caller.
		 */
		/* XXX: possibly have callback in fsm struct, instead. like the colour hooks */
		if (end != NULL && carryopaque != NULL) {
			if (fsm_isend(fsm, fsm->start)) {
				struct set *set;

				set = NULL;

				if (!set_add(&set, new->start)) {
					set_free(set);
					fsm_free(new);
					return 0;
				}

				carryopaque(set, new, end);

				set_free(set);
			}
		}
	}

	/* Create reversed edges */
	{
		struct fsm_state *s;

		for (s = fsm->sl; s != NULL; s = s->next) {
			struct fsm_state *to;
			struct fsm_state *se;
			struct set_iter it;
			struct fsm_edge *e;

			to = equivalent(new, fsm, s);

			assert(to != NULL);

			for (e = set_first(s->edges, &it); e != NULL; e = set_next(&it)) {
				struct set_iter jt;

				for (se = set_first(e->sl, &jt); se != NULL; se = set_next(&jt)) {
					struct fsm_state *from;
					struct fsm_edge *edge;

					assert(se != NULL);

					from = equivalent(new, fsm, se);

					assert(from != NULL);

					edge = fsm_addedge(from, to, e->symbol);
					if (edge == NULL) {
						fsm_free(new);
						return 0;
					}
				}
			}
		}
	}

	/*
	 * Mark the new start state. If there's only one state, we can indicate it
	 * directly. Otherwise we will be starting from a group of states, linked
	 * together by epsilon transitions.
	 */
	{
		struct fsm_state *s;
		unsigned n;

		n = fsm_count(fsm, fsm_isend);

		switch (n) {
		case 1:
			for (s = fsm->sl; s != NULL; s = s->next) {
				if (fsm_isend(fsm, s)) {
					break;
				}
			}

			assert(s != NULL);

			new->start = equivalent(new, fsm, s);
			assert(new->start != NULL);
			break;

		case 0:
			/*
			 * Transition to all states. I don't like this at all, but it is
			 * required to be able to minimise automata with no end states.
			 */

		default:
			if (!fsm->tidy) {
				s = NULL;
			} else {
				for (s = fsm->sl; s != NULL; s = s->next) {
					struct fsm_state *state;

					if (n > 0 && !fsm_isend(fsm, s)) {
						continue;
					}

					state = equivalent(new, fsm, s);
					assert(state != NULL);

					if (!fsm_hasincoming(new, state)) {
						break;
					}
				}
			}

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
			 * disabled by the fsm->tidy flag.
			 */
			if (s != NULL) {
				new->start = equivalent(new, fsm, s);
				assert(new->start != NULL);
			} else {
				new->start = fsm_addstate(new);
				if (new->start == NULL) {
					fsm_free(new);
					return 0;
				}
			}

			for (s = fsm->sl; s != NULL; s = s->next) {
				struct fsm_state *state;

				if (n > 0 && !fsm_isend(fsm, s)) {
					continue;
				}

				state = equivalent(new, fsm, s);
				assert(state != NULL);

				if (state == new->start) {
					continue;
				}

				if (!fsm_addedge_epsilon(new, new->start, state)) {
					fsm_free(new);
					return 0;
				}
			}
			break;
		}
	}

	fsm_move(fsm, new);

	return 1;
}

int
fsm_reverse(struct fsm *fsm)
{
	return fsm_reverse_opaque(fsm, NULL);
}

