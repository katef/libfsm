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
	fsm_state_t sa, sb;
	fsm_state_t sq;
	fsm_state_t base_a, base_b;
	int ia, ib;

	assert(a != NULL);
	assert(b != NULL);

	if (a->opt != b->opt) {
		errno = EINVAL;
		return NULL;
	}

	if (a->statecount == 0) { return b; }
	if (b->statecount == 0) { return a; }

	if (!fsm_getstart(a, &sa) || !fsm_getstart(b, &sb)) {
		errno = EINVAL;
		return NULL;
	}

	q = fsm_merge(a, b, &base_a, &base_b);
	assert(q != NULL);

	sa += base_a;
	sb += base_b;

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
		if (!fsm_addstate(q, &sq)) {
			goto error;
		}
	} else {
		ia = fsm_hasincoming(q, sa);
		ib = fsm_hasincoming(q, sb);

		if (!ia && !ib) {
			if (!fsm_mergestates(q, sa, sb, &sq)) {
				goto error;
			}
			sa = sb = sq;
		} else if (!ia) {
			sq = sa;
		} else if (!ib) {
			sq = sb;
		} else {
			if (!fsm_addstate(q, &sq)) {
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

