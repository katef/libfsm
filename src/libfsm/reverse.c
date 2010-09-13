/* $Id$ */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/graph.h>

#include "internal.h"
#include "set.h"

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
		struct opaque_set *o;

		for (s = fsm->sl; s; s = s->next) {
			struct fsm_state *n;

			n = fsm_addstateid(new, s->id);
			if (n == NULL) {
				fsm_free(new);
				return 0;
			}

			for (o = s->ol; o; o = o->next) {
				if (!fsm_addopaque(new, n, o->opaque)) {
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

		for (s = fsm->sl; s; s = s->next) {
			struct fsm_state *to;
			struct state_set *e;
			int i;

			to = fsm_getstatebyid(new, s->id);

			assert(to != NULL);

			for (i = 0; i <= FSM_EDGE_MAX; i++) {
				for (e = s->edges[i]; e; e = e->next) {
					struct fsm_state *from;

					assert(e->state != NULL);

					from = fsm_getstatebyid(new, e->state->id);

					assert(from != NULL);

					if (!fsm_addedge_literal(new, from, to, i)) {
						fsm_free(new);
						return 0;
					}
				}
			}
		}
	}

	/* Create reverse epsilon transitions */
	{
		struct fsm_state *s;
		struct state_set *e;

		for (s = fsm->sl; s; s = s->next) {
			struct fsm_state *to = fsm_getstatebyid(new, s->id);

			assert(to != NULL);

			for (e = s->el; e; e = e->next) {
				struct fsm_state *from;

				assert(e->state != NULL);

				from = fsm_getstatebyid(new, e->state->id);

				assert(from != NULL);

				if (!fsm_addedge_epsilon(new, from, to)) {
					fsm_free(new);
					return 0;
				}
			}
		}
	}

	/* Create the new start state */
	{
		struct fsm_state *s;
		int endcount;

		endcount = 0;
		for (s = fsm->sl; s; s = s->next) {
			endcount += !!fsm_isend(fsm, s);
		}

		switch (endcount) {
		case 0:
			/* Start from an epsilon transition to all states */
			{
				struct fsm_state *start;

				start = fsm_addstate(new);
				if (start == NULL) {
					fsm_free(new);
					return 0;
				}

				fsm_setstart(new, start);

				for (s = new->sl; s; s = s->next) {
					if (s == start) {
						continue;
					}

					fsm_addedge_epsilon(new, start, s);
				}
			}
			break;

		case 1:
			/* Since there's only one state, we can indicate it directly */
			for (s = fsm->sl; s; s = s->next) {
				if (fsm_isend(fsm, s)) {
					struct fsm_state *start;

					start = fsm_getstatebyid(new, s->id);
					assert(start != NULL);

					fsm_setstart(new, start);
				}
			}
			break;

		default:
			/* Start from an epsilon transition to each end state */
			{
				struct fsm_state *start;

				start = fsm_addstate(new);
				if (start == NULL) {
					fsm_free(new);
					return 0;
				}

				fsm_setstart(new, start);

				for (s = fsm->sl; s; s = s->next) {
					struct fsm_state *state;

					if (s == start) {
						continue;
					}

					if (!fsm_isend(fsm, s)) {
						continue;
					}

					state = fsm_getstatebyid(new, s->id);
					assert(state != NULL);

					fsm_addedge_epsilon(new, start, state);
				}
			}
			break;
		}
	}

	fsm_move(fsm, new);

	return 1;
}

