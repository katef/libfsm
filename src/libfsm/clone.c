/* $Id$ */

#include <assert.h>
#include <stdlib.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include "internal.h"

/* TODO: centralise? or optimise by folding into the loops below? */
static struct fsm_state *
equivalent(const struct fsm *a, const struct fsm *b,
	const struct fsm_state *state)
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
fsm_clone(const struct fsm *fsm)
{
	struct fsm *new;

	assert(fsm != NULL);

	new = fsm_new();
	if (new == NULL) {
		return NULL;
	}

	/*
	 * Create states corresponding to the origional FSM's states.
	 * These are created in reverse order, but that's okay.
	 */
	/* TODO: possibly centralise as a state-cloning function */
	{
		struct fsm_state *s;

		for (s = fsm->sl; s != NULL; s = s->next) {
			struct fsm_state *q;

			q = fsm_addstate(new);
			if (q == NULL) {
				fsm_free(new);
				return NULL;
			}
		}
	}

	{
		struct fsm_state *s, *to;

		for (s = fsm->sl; s != NULL; s = s->next) {
			struct fsm_state *equiv;
			struct fsm_edge *e;
			struct set_iter it;

			equiv = equivalent(fsm, new, s);
			assert(equiv != NULL);

			if (*fsm->tail == s) {
				new->tail = &equiv;
			}

			fsm_setend(new, equiv, fsm_isend(fsm, s));
			equiv->opaque = s->opaque;

			for (e = set_first(s->edges, &it); e != NULL; e = set_next(&it)) {
				struct set_iter jt;

				for (to = set_first(e->sl, &jt); to != NULL; to = set_next(&jt)) {
					struct fsm_state *newfrom;
					struct fsm_state *newto;

					newfrom = equiv;
					newto   = equivalent(fsm, new, to);

					if (!fsm_addedge(newfrom, newto, e->symbol)) {
						fsm_free(new);
						return NULL;
					}
				}
			}
		}
	}

	new->start = equivalent(fsm, new, fsm->start);

	new->tidy = fsm->tidy;

	return new;
}

