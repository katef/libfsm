/* $Id$ */

#include <assert.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include "internal.h"
#include "set.h"

/* TODO: centralise? or optimise by folding into the loops below? */
static struct fsm_state *
equivalent(const struct fsm *a, const struct fsm *b, const struct fsm_state *state)
{
	struct fsm_state *p;
	struct fsm_state *q;

	assert(a != NULL);
	assert(b != NULL);

	if (state == NULL) {
		return NULL;
	}

	for (p = a->sl, q = b->sl; p != NULL && q != NULL; p = p->next, q = q->next) {
		if (p == state) {
			return q;
		}
	}

	return NULL;
}

struct fsm *
fsm_copy(const struct fsm *fsm)
{
	struct fsm *new;

	assert(fsm != NULL);

	new = fsm_new();
	if (new == NULL) {
		return NULL;
	}

	fsm_setcolourhooks(new, &fsm->colour_hooks);

	/*
	 * Create states corresponding to the origional FSM's states.
	 * These are created in reverse order, but that's okay.
	 */
	/* TODO: possibly centralise as a state-copying function */
	{
		struct fsm_state *s;

		for (s = fsm->sl; s != NULL; s = s->next) {
			if (!fsm_addstate(new)) {
				fsm_free(new);
				return NULL;
			}
		}
	}

	{
		struct colour_set *c;
		struct state_set *to;
		struct fsm_state *s;
		int i;

		for (s = fsm->sl; s != NULL; s = s->next) {
			for (c = s->cl; c != NULL; c = c->next) {
				if (!fsm_addend(new, equivalent(fsm, new, s), c->colour)) {
					fsm_free(new);
					return NULL;
				}
			}

			for (i = 0; i <= FSM_EDGE_MAX; i++) {
				for (to = s->edges[i].sl; to != NULL; to = to->next) {
					struct fsm_state *newfrom;
					struct fsm_state *newto;

					newfrom = equivalent(fsm, new, s);
					newto   = equivalent(fsm, new, to->state);

					if (!set_addstate(&newfrom->edges[i].sl, newto)) {
						fsm_free(new);
						return NULL;
					}
				}
			}
		}
	}

	new->start = equivalent(fsm, new, fsm->start);

	return new;
}

