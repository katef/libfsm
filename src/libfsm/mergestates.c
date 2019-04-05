/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include <fsm/fsm.h>

#include "internal.h"

struct fsm_state *
fsm_mergestates(struct fsm *fsm, struct fsm_state *a, struct fsm_state *b)
{
	struct fsm_edge *e;
	struct fsm_state *s;
	struct edge_iter it;

	/* edges from b */
	for (e = edge_set_first(b->edges, &it); e != NULL; e = edge_set_next(&it)) {
		struct state_iter jt;

		for (s = state_set_first(e->sl, &jt); s != NULL; s = state_set_next(&jt)) {
			if (e->symbol > UCHAR_MAX) {
				assert(e->symbol == FSM_EDGE_EPSILON);
				if (!fsm_addedge_epsilon(fsm, a, s)) {
					return NULL;
				}
			} else {
				if (!fsm_addedge_literal(fsm, a, s, e->symbol)) {
					return NULL;
				}
			}
		}
	}

	/* edges to b */
	for (s = fsm->sl; s != NULL; s = s->next) {
		for (e = edge_set_first(s->edges, &it); e != NULL; e = edge_set_next(&it)) {
			if (!state_set_contains(e->sl, b)) {
				continue;
			}

			if (e->symbol > UCHAR_MAX) {
				assert(e->symbol == FSM_EDGE_EPSILON);
				if (!fsm_addedge_epsilon(fsm, s, a)) {
					return NULL;
				}
			} else {
				if (!fsm_addedge_literal(fsm, s, a, e->symbol)) {
					return NULL;
				}
			}

			state_set_remove(e->sl, b);
		}
	}

	fsm_removestate(fsm, b);

	return a;
}

