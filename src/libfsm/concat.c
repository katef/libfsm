/* $Id$ */

#include <assert.h>
#include <stddef.h>
#include <limits.h>
#include <errno.h>

#include <adt/set.h>

#include <fsm/pred.h>
#include <fsm/graph.h>

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
static int
nonepsilonedges(const struct fsm *fsm, const struct fsm_state *state)
{
	int i;

	assert(fsm != NULL);
	assert(state != NULL);

	for (i = 0; i <= UCHAR_MAX; i++) {
		if (state->edges[i].sl != NULL) {
			return 1;
		}
	}

	return 0;
}

/* TODO: centralise */
static int
outgoingedges(const struct fsm *fsm, const struct fsm_state *state)
{
	int i;

	assert(fsm != NULL);
	assert(state != NULL);

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		if (state->edges[i].sl != NULL) {
			return 1;
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
fsm_concat(struct fsm *a, struct fsm *b)
{
	struct fsm *q;
	struct fsm_state *ea, *sb;
	struct fsm_state *sq;

	assert(a != NULL);
	assert(b != NULL);

	if (!fsm_hasend(a)) {
		errno = EINVAL;
		return NULL;
	}

	sq = fsm_getstart(a);
	sb = fsm_getstart(b);
	if (sq == NULL || sb == NULL) {
		errno = EINVAL;
		return NULL;
	}

	ea = fsm_collate(a, fsm_isend);
	if (ea == NULL) {
		return NULL;
	}

	fsm_setend(a, ea, 1);

	q = fsm_merge(a, b);
	assert(q != NULL);

	fsm_setend(q, ea, 0);

	/*
	 * The canonical approach is to create epsilon transition(s) from the end
	 * state of one FSM to the start state of the next FSM.
	 *
	 * TODO: diagram
	 *
	 * In this implementation, if multiple end states are present, they are
	 * first collated together by epsilon transitions to a single end state.
	 * Then that single state is linked to the next FSM.
	 *
	 * This is always semantically correct, however in some situations,
	 * adding the epsilon transiion is unneccessary. As an optimisation,
	 * we identify situations where it would be equivalent to merge the
	 * two states instead.
	 */

	if (!outgoingedges(q, ea) || !incomingedges(q, sb)) {
		if (!state_merge(q, ea, sb)) {
			goto error;
		}
	} else {
		if (!fsm_addedge_epsilon(q, ea, sb)) {
			goto error;
		}
	}

	fsm_setstart(q, sq);

	return q;

error:

	fsm_free(q);

	return NULL;
}

