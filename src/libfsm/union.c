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
fsm_union(struct fsm *a, struct fsm *b,
	struct fsm_combine_info *combine_info)
{
	struct fsm *q;
	fsm_state_t sa, sb;
	fsm_state_t sq;
	struct fsm_combine_info combine_info_internal;

	if (combine_info == NULL) {
		combine_info = &combine_info_internal;
	}

	assert(a != NULL);
	assert(b != NULL);

	if (a->opt != b->opt) {
		errno = EINVAL;
		return NULL;
	}

	if (a->statecount == 0) { fsm_free(a); return b; }
	if (b->statecount == 0) { fsm_free(b); return a; }

	if (!fsm_getstart(a, &sa) || !fsm_getstart(b, &sb)) {
		errno = EINVAL;
		return NULL;
	}

	q = fsm_merge(a, b, combine_info);
	assert(q != NULL);

	sa += combine_info->base_a;
	sb += combine_info->base_b;

	/*
	 * The canonical approach is to create a new start state, with epsilon
	 * transitions to both a->start and b->start.
	 *
	 * TODO: diagram
	 */

	if (!fsm_addstate(q, &sq)) {
		goto error;
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

