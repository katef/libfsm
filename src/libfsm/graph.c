/* $Id$ */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/graph.h>

#include "internal.h"

struct fsm *
fsm_reverse(struct fsm *fsm)
{
	struct fsm *new;

	assert(fsm != NULL);

	new = fsm_new();
	if (new == NULL) {
		return NULL;
	}

	fsm_setoptions(new, &fsm->options);

	/* create states corresponding to the origional FSM's states */
	{
		struct state_list *s;

		for (s = fsm->sl; s; s = s->next) {
			if (fsm_addstate(new, s->state.id, 0) == NULL) {
				fsm_free(new);
				return NULL;
			}
		}
	}

	/*
	 * The new end state is the previous start state. Because there is exactly
	 * one start state, the new FSM will have exactly one end state.
	 */
	fsm_setend(new, fsm_getstatebyid(new, fsm->start->id), 1);

	/* Create reversed edges */
	{
		struct state_list *s;
		struct fsm_edge   *e;

		for (s = fsm->sl; s; s = s->next) {
			struct fsm_state *to = fsm_getstatebyid(new, s->state.id);

			assert(to != NULL);

			for (e = s->state.edges; e; e = e ->next) {
				struct fsm_state *from = fsm_getstatebyid(new, e->state->id);

				assert(from != NULL);

				if (fsm_addedge(new, from, to, e->label->label) == NULL) {
					fsm_free(new);
					return NULL;
				}
			}
		}
	}

	/* Create the new start state */
	{
		struct state_list *s;
		int endcount;

		endcount = 0;
		for (s = fsm->sl; s; s = s->next) {
			endcount += !!fsm_isend(fsm, &s->state);
		}

		switch (endcount) {
		case 0:
			/* Start from an epsilon transition to all states */
			{
				struct fsm_state *start;

				start = fsm_addstate(new, 0, 0);
				if (start == NULL) {
					return NULL;
				}

				fsm_setstart(new, start);

				for (s = new->sl; s; s = s->next) {
					if (&s->state == start) {
						continue;
					}

					fsm_addedge(new, start, &s->state, NULL);
				}
			}
			break;

		case 1:
			/* Since there's only one state, we can indicate it directly */
			for (s = fsm->sl; s; s = s->next) {
				if (fsm_isend(new, &s->state)) {
					fsm_setstart(new, &s->state);
				}
			}
			break;

		default:
			/* Start from an epsilon transition to each end state */
			{
				struct fsm_state *start;

				start = fsm_addstate(new, 0, 0);
				if (start == NULL) {
					return NULL;
				}

				fsm_setstart(new, start);

				for (s = new->sl; s; s = s->next) {
					if (&s->state == start) {
						continue;
					}

					if (!fsm_isend(fsm, fsm_getstatebyid(fsm, s->state.id))) {
						continue;
					}

					fsm_addedge(new, start, &s->state, NULL);
				}
			}
			break;
		}
	}

	fsm_move(fsm, new);

	return fsm;
}

