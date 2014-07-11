/* $Id$ */

#include <assert.h>
#include <stddef.h>
#include <limits.h>
#include <errno.h>

#include <adt/set.h>

#include <fsm/bool.h>

#include "internal.h"

/* TODO: centralise */
static int
incomingedges(const struct fsm *fsm, const struct fsm_state *state)
{
	const struct fsm_state *s;
	const struct state_set *e;
	int i;

	assert(fsm != NULL);
	assert(state != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			for (e = state->edges[i].sl; e != NULL; e = e->next) {
				if (e->state == state) {
					return 1;
				}
			}
		}
	}

	return 0;
}

/* TODO: centralise */
static void
set_merge(struct state_set **dst, struct state_set *src)
{
	struct state_set **p;

	assert(dst != NULL);

	for (p = dst; *p != NULL; p = &(*p)->next) {
		/* nothing */
	}

	*p = src;
}

/* TODO: centralise */
static struct fsm_state *
state_merge(struct fsm *q, struct fsm_state *a, struct fsm_state *b)
{
	int i;

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		/* there should be no common elements, because a and b are from different FSMs */
		/* TODO: make a set_distinct() or set_disjoint() or somesuch */
		assert(a->edges[i].sl == NULL || !subsetof(a->edges[i].sl, b->edges[i].sl));
		assert(b->edges[i].sl == NULL || !subsetof(b->edges[i].sl, a->edges[i].sl));

		set_merge(&a->edges[i].sl, b->edges[i].sl);

		b->edges[i].sl = NULL;
	}

	/* TODO: could just use internal state_remove(), since no edges transition to to b */
	fsm_removestate(q, b);

	return a;
}

struct fsm *
fsm_union(struct fsm *a, struct fsm *b)
{
	struct fsm *q;
	struct fsm_state *sa, *sb;
	struct fsm_state *sq;
	int ia, ib;

	assert(a != NULL);
	assert(b != NULL);

	sa = fsm_getstart(a);
	sb = fsm_getstart(b);
	if (sa == NULL || sb == NULL) {
		errno = EINVAL;
		return NULL;
	}

	q = fsm_merge(a, b);
	assert(q != NULL);

	/*
	 * The canonical approach is to create a new start state, with epsilon
	 * transitions to both a->start and b->start.
	 *
	 * TODO: diagram
	 *
	 * This is always semantically correct, however in some situations,
	 * adding the extra state is unneccessary. As an optimisation, we nominate
	 * a->start or b->start to serve as the new start state where possible.
	 * If both a->start and b->start are suitable, then their edges
	 * are merged into one state, and the other removed.
	 *
	 * a->start and b->start are considered suitable to serve as the start
	 * state if they have no incoming edges.
	 */

	ia = incomingedges(q, sa);
	ib = incomingedges(q, sb);

	if (!ia && !ib) {
		sq = state_merge(q, sa, sb);
		sa = sb = sq;
	} else if (!ia) {
		sq = sa;
	} else if (!ib) {
		sq = sb;
	} else {
		sq = fsm_addstate(q);
		if (sq == NULL) {
			goto error;
		}
	}

	fsm_setstart(q, sq);

	if (sq != sa) {
		if (!fsm_addedge_epsilon(q, sq, sa)) {
			goto error;
		}
	}

	if (sq != sb) {
		if (!fsm_addedge_epsilon(q, sq, sb)) {
			goto error;
		}
	}

	return q;

error:

	fsm_free(q);

	return NULL;
}

