/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include "internal.h"

struct fsm *
fsm_clone(const struct fsm *fsm)
{
	struct fsm *new;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	new = fsm_new(fsm->opt);
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

			s->equiv = q;
		}
	}

	{
		struct fsm_state *s, *to;

		for (s = fsm->sl; s != NULL; s = s->next) {
			struct fsm_state *equiv;
			struct fsm_edge *e;
			struct set_iter it;

			equiv = s->equiv;
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
					newto   = to->equiv;

					if (!fsm_addedge(new, newfrom, newto, e->symbol)) {
						fsm_free(new);
						return NULL;
					}
				}
			}
		}
	}

	new->start = fsm->start->equiv;

	return new;
}

