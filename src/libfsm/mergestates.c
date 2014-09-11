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
		/* there should be no common elements, because a and b are from different FSMs */
		/* TODO: make a set_distinct() or set_disjoint() or somesuch */
		assert(a->edges[i].sl == NULL || !subsetof(a->edges[i].sl, b->edges[i].sl));
		assert(b->edges[i].sl == NULL || !subsetof(b->edges[i].sl, a->edges[i].sl));

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

