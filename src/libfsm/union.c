/* $Id$ */

#include <assert.h>
#include <stddef.h>
#include <limits.h>

#include <fsm/bool.h>

#include "internal.h"

struct fsm *
fsm_union(struct fsm *a, struct fsm *b)
{
	struct fsm *q;
	struct fsm_state *sa, *sb;
	struct fsm_state *sq;

	assert(a != NULL);
	assert(b != NULL);

	/* TODO: diagram */

	sa = a->start;
	sb = b->start;

	q = fsm_merge(a, b);
	assert(q != NULL);

	sq = fsm_addstate(q);
	if (sq == NULL) {
		goto error;
	}

	fsm_setstart(q, sq);

	if (!fsm_addedge_epsilon(q, sq, sa)) {
		goto error;
	}

	if (!fsm_addedge_epsilon(q, sq, sb)) {
		goto error;
	}

	return q;

error:

	fsm_free(q);

	return NULL;
}

