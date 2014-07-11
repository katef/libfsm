/* $Id$ */

#include <assert.h>
#include <stddef.h>
#include <limits.h>
#include <errno.h>

#include <adt/set.h>

#include <fsm/bool.h>
#include <fsm/graph.h>

#include "internal.h"

struct fsm *
fsm_subtract(struct fsm *a, struct fsm *b)
{
	struct fsm *q;

	/*
	 * a - b is implemented as: a & ~b
	 */

	if (!fsm_complement(b)) {
		return NULL;
	}

	q = fsm_intersect(a, b);
	if (q == NULL) {
		return NULL;
	}

	return q;
}

