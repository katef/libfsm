/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <limits.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/options.h>

#include "internal.h"

struct fsm *
fsm_union(struct fsm *a, struct fsm *b)
{
	struct fsm *q;
	struct fsm_state *sa, *sb;
	struct fsm_state *sq;
	int ia, ib;

	assert(a != NULL);
	assert(b != NULL);

	if (a->opt != b->opt) {
		errno = EINVAL;
		return NULL;
	}

	if (a->statecount == 0) { return b; }
	if (b->statecount == 0) { return a; }

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
	 *
	 * This optimisation can be expensive to run, so it's optionally disabled
	 * by the opt->tidy flag.
	 */

	if (!q->opt->tidy) {
		sq = fsm_addstate(q);
		if (sq == NULL) {
			goto error;
		}
	} else {
		ia = fsm_hasincoming(q, sa);
		ib = fsm_hasincoming(q, sb);

		if (!ia && !ib) {
			sq = fsm_mergestates(q, sa, sb);
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

