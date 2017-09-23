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

#include "internal.h"
#include "walk2.h"

struct fsm *
fsm_subtract(struct fsm *a, struct fsm *b)
{
	struct fsm *q;

	assert(a != NULL);
	assert(b != NULL);

	if (a->opt != b->opt) {
		errno = EINVAL;
		return NULL;
	}

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

