/* $Id$ */

#include <assert.h>
#include <stddef.h>
#include <limits.h>
#include <errno.h>

#include <fsm/graph.h>

#include "internal.h"

struct fsm *
fsm_concat(struct fsm *a, struct fsm *b)
{
	struct fsm *q;
	struct fsm_state *ea, *sb;
	struct fsm_state *sq;

	assert(a != NULL);
	assert(b != NULL);

	/* TODO: diagram */

	if (!fsm_hasend(a)) {
		errno = EINVAL;
		return NULL;
	}

	ea = fsm_collateends(a);
	if (ea == NULL) {
		return NULL;
	}

	sb = fsm_getstart(b);
	if (sb == NULL) {
		return NULL;
	}

	sq = fsm_getstart(a);
	if (sq == NULL) {
		return NULL;
	}

	q = fsm_merge(a, b);
	assert(q != NULL);

	fsm_setstart(q, sq);

	fsm_setend(q, ea, 0);

	if (!fsm_addedge_epsilon(q, ea, sb)) {
		goto error;
	}

	return q;

error:

	fsm_free(q);

	return NULL;
}

