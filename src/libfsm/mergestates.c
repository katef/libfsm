/* $Id$ */

#include <assert.h>
#include <stdlib.h>

#include <adt/set.h>

#include <fsm/fsm.h>

#include "internal.h"

struct fsm_state *
fsm_mergestates(struct fsm *fsm, struct fsm_state *a, struct fsm_state *b)
{
	struct fsm_state *s;
	int i;

	/* edges from b */
	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		struct state_set *p;

		/* set_merge assumes no common elements */
		for (p = b->edges[i].sl; p != NULL; p = p->next) {
			set_remove(&a->edges[i].sl, p->state);
		}

		set_merge(&a->edges[i].sl, b->edges[i].sl);

		b->edges[i].sl = NULL;
	}

	/* edges to b */
	for (s = fsm->sl; s != NULL; s = s->next) {
		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			if (set_contains(s->edges[i].sl, b)) {
				if (!set_addstate(&s->edges[i].sl, a)) {
					return NULL;
				}

				set_remove(&s->edges[i].sl, b);
			}
		}
	}

	fsm_removestate(fsm, b);

	return a;
}

