/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <adt/set.h>

#include <fsm/fsm.h>

#include "internal.h"

struct fsm_state *
fsm_mergestates(struct fsm *fsm, struct fsm_state *a, struct fsm_state *b)
{
	struct fsm_edge *e, *f;
	struct fsm_state *s;
	struct set_iter it;

	/* edges from b */
	for (e = set_first(b->edges, &it); e != NULL; e = set_next(&it)) {
		struct set_iter jt;
		for (s = set_first(e->sl, &jt); s != NULL; s = set_next(&jt)) {
			f = fsm_addedge(&fsm->alloc, a, s, e->symbol);
			if (f == NULL) {
				return NULL;
			}
		}
	}

	/* edges to b */
	for (s = fsm->sl; s != NULL; s = s->next) {
		for (e = set_first(s->edges, &it); e != NULL; e = set_next(&it)) {
			if (set_contains(e->sl, b)) {
				f = fsm_addedge(&fsm->alloc, s, a, e->symbol);
				if (f == NULL) {
					return NULL;
				}
				set_remove(&e->sl, b);
			}
		}
	}

	fsm_removestate(fsm, b);

	return a;
}

