/* $Id$ */

#include <assert.h>
#include <stdlib.h>
#include <string.h>	/* XXX */

#include <fsm/fsm.h>
#include <fsm/graph.h>

#include "internal.h"
#include "set.h"
#include "colour.h"

static void
removeorphans(struct fsm *fsm, struct fsm_state *state)
{
	struct fsm_state *s;
	int i;

	assert(fsm != NULL);
	assert(state != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			if (set_contains(state, s->edges[i].sl)) {
				return;
			}
		}
	}

	fsm_removestate(fsm, state);
}

static int
splitstate(struct fsm *fsm, void *colour, struct fsm_state *state)
{
	struct fsm_state *new;
	struct fsm_state *s;
	struct state_set *p;
	int i;

	assert(fsm != NULL);
	assert(state != NULL);

	new = fsm_addstate(fsm);
	if (new == NULL) {
		return 0;
	}

	/* copy end colours; XXX: this should probably be a subset */
	{
		struct colour_set *c;

		for (c = state->cl; c != NULL; c = c->next) {
			if (!fsm_addend(fsm, new, c->colour)) {
				return 0;
			}
		}
	}

	/* copy incoming edges */
	for (s = fsm->sl; s != NULL; s = s->next) {
		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			if (!set_contains(state, s->edges[i].sl)) {
				continue;
			}

			if (set_containscolour(fsm, colour, s->edges[i].cl)) {
				set_replace(s->edges[i].sl, state, new);
			}
		}
	}

	/* copy outgoing edges */
	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		if (set_containscolour(fsm, colour, state->edges[i].cl)) {
			for (p = state->edges[i].sl; p != NULL; p = p->next) {
				if (!set_addstate(&new->edges[i].sl, p->state)) {
					return 0;
				}
			}

			set_removecolour(fsm, &state->edges[i].cl, colour);

			if (!set_addcolour(fsm, &new->edges[i].cl, colour)) {
				return 0;
			}
		}
	}

	removeorphans(fsm, new);

	return 1;
}

static int
splitcolour(struct fsm *fsm, void *colour)
{
	struct fsm_state *s;
	struct fsm_state *next;

	assert(fsm != NULL);

	for (s = fsm->sl; s != NULL; s = next) {
		next = s->next;

		if (fsm_ispure(fsm, s, colour)) {
			continue;
		}

		if (!splitstate(fsm, colour, s)) {
			return 0;
		}
	}

	return 1;
}

int
fsm_split(struct fsm *fsm)
{
	struct colour_set *c;
	struct fsm_state *s;
	int i;

	assert(fsm != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			for (c = s->edges[i].cl; c != NULL; c = c->next) {
				if (!splitcolour(fsm, c->colour)) {
					return 0;
				}
			}
		}
	}

	/* TODO: if no longer a DFA, then you have ambigious regexps unioned */

	return 1;
}

