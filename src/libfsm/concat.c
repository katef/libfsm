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
#include <fsm/walk.h>

#include "internal.h"

struct fsm *
fsm_concat(struct fsm *a, struct fsm *b,
	struct fsm_combine_info *combine_info)
{
	struct fsm *q;
	fsm_state_t ea, sb;
	fsm_state_t sq;
	struct fsm_combine_info combine_info_internal;

	if (combine_info == NULL) {
		combine_info = &combine_info_internal;
	}

	assert(a != NULL);
	assert(b != NULL);

	if (a->alloc != b->alloc) {
		errno = EINVAL;
		return NULL;
	}

	if (a->statecount == 0) { return b; }
	if (b->statecount == 0) { return a; }

	if (!fsm_has(a, fsm_isend)) {
		errno = EINVAL;
		return NULL;
	}

	if (!fsm_getstart(a, &sq) || !fsm_getstart(b, &sb)) {
		errno = EINVAL;
		return NULL;
	}

	if (!fsm_collate(a, &ea, fsm_isend)) {
		return NULL;
	}

	fsm_setend(a, ea, 1);

	q = fsm_merge(a, b, combine_info);
	assert(q != NULL);

	sq += combine_info->base_a;
	ea += combine_info->base_a;
	sb += combine_info->base_b;

	fsm_setend(q, ea, 0);

	/*
	 * The textbook approach is to create epsilon transition(s) from the end
	 * state of one FSM to the start state of the next FSM.
	 *
	 *     a: ⟶ ○ ··· ◎
	 *                       a b: ⟶ ○ ··· ○ ⟶ ○ ··· ◎
	 *     b: ⟶ ○ ··· ◎                a         b   
	 *
	 * In this implementation, if multiple end states are present, they are
	 * first collated together by epsilon transitions to a single end state.
	 * Then that single state is linked to the next FSM.
	 */

	if (!fsm_addedge_epsilon(q, ea, sb)) {
		goto error;
	}

	fsm_setstart(q, sq);

	return q;

error:

	fsm_free(q);

	return NULL;
}

