/* $Id$ */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/graph.h>

#include "internal.h"
#include "set.h"
#include "colour.h"

/* TODO: centralise? */
static int incomingedges(struct fsm *fsm, struct fsm_state *state) {
	struct fsm_state *s;
	int i;

	for (s = fsm->sl; s != NULL; s = s->next) {
		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			if (set_contains(state, s->edges[i].sl)) {
				return 1;
			}
		}
	}

	return 0;
}

int
fsm_reverse(struct fsm *fsm)
{
	struct fsm *new;

	assert(fsm != NULL);

	new = fsm_new();
	if (new == NULL) {
		return 0;
	}

	fsm_setoptions(new, &fsm->options);

	/*
	 * Create states corresponding to the origional FSM's states. Their opaque
	 * values are copied over.
	 *
	 * TODO: possibly centralise with fsm_copy() into a state-copying function
	 */
	{
		struct fsm_state *s;
		struct colour_set *c;

		for (s = fsm->sl; s != NULL; s = s->next) {
			struct fsm_state *n;

			n = fsm_addstateid(new, s->id);
			if (n == NULL) {
				fsm_free(new);
				return 0;
			}

			for (c = s->cl; c != NULL; c = c->next) {
				if (!fsm_addcolour(new, n, c->colour)) {
					fsm_free(new);
					return 0;
				}
			}
		}
	}

	/*
	 * The new end state is the previous start state. Because there is exactly
	 * one start state, the new FSM will have exactly one end state.
	 */
	{
		struct fsm_state *end;

		end = fsm_getstatebyid(new, fsm->start->id);
		assert(end != NULL);
		fsm_setend(new, end, 1);
	}

	/* Create reversed edges */
	{
		struct fsm_state *s;

		for (s = fsm->sl; s != NULL; s = s->next) {
			struct fsm_state *to;
			struct state_set *e;
			int i;

			to = fsm_getstatebyid(new, s->id);

			assert(to != NULL);

			for (i = 0; i <= FSM_EDGE_MAX; i++) {
				for (e = s->edges[i].sl; e != NULL; e = e->next) {
					struct fsm_state *from;

					assert(e->state != NULL);

					from = fsm_getstatebyid(new, e->state->id);

					assert(from != NULL);

					switch (i) {
					case FSM_EDGE_EPSILON:
						if (!fsm_addedge_epsilon(new, from, to)) {
							fsm_free(new);
							return 0;
						}
						break;

					default:
						assert(i >= 0);
						assert(i <= UCHAR_MAX);

						if (!fsm_addedge_literal(new, from, to, i)) {
							fsm_free(new);
							return 0;
						}
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
		int endcount;

		endcount = 0;
		for (s = fsm->sl; s != NULL; s = s->next) {
			endcount += !!fsm_isend(fsm, s);
		}

		switch (endcount) {
		case 1:
			for (s = fsm->sl; s != NULL; s = s->next) {
				if (fsm_isend(fsm, s)) {
					break;
				}
			}

			assert(s != NULL);

			new->start = fsm_getstatebyid(new, s->id);
			assert(new->start != NULL);
			break;

		case 0:
			/*
			 * Transition to all states. I don't like this at all, but it is
			 * required to be able to minimize automata with no end states.
			 */

		default:
			for (s = fsm->sl; s != NULL; s = s->next) {
				struct fsm_state *state;

				if (endcount > 0 && !fsm_isend(fsm, s)) {
					continue;
				}

				state = fsm_getstatebyid(new, s->id);
				assert(state != NULL);

				if (!incomingedges(new, state)) {
					break;
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
			 */
			if (s != NULL) {
				new->start = fsm_getstatebyid(new, s->id);
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

				if (endcount > 0 && !fsm_isend(fsm, s)) {
					continue;
				}

				if (s->id == new->start->id) {
					continue;
				}

				state = fsm_getstatebyid(new, s->id);
				assert(state != NULL);

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

