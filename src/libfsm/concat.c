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
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/options.h>

#include "internal.h"

struct fsm *
fsm_concat(struct fsm *a, struct fsm *b)
{
	struct fsm *q;
	struct fsm_state *ea, *sb;
	struct fsm_state *sq;

	assert(a != NULL);
	assert(b != NULL);

	if (a->opt != b->opt) {
		errno = EINVAL;
		return NULL;
	}

	if (a->statecount == 0) { return b; }
	if (b->statecount == 0) { return a; }

	if (!fsm_has(a, fsm_isend)) {
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
	 *
	 * This optimisation can be expensive to run, so it's optionally disabled
	 * by the opt->tidy flag.
	 */

	if (!q->opt->tidy) {
		if (!fsm_addedge_epsilon(q, ea, sb)) {
			goto error;
		}
	} else {
		if (!fsm_hasoutgoing(q, ea) || !fsm_hasincoming(q, sb)) {
			if (!fsm_mergestates(q, ea, sb)) {
				goto error;
			}
		} else {
			if (!fsm_addedge_epsilon(q, ea, sb)) {
				goto error;
			}
		}
	}

	fsm_setstart(q, sq);

	return q;

error:

	fsm_free(q);

	return NULL;
}

