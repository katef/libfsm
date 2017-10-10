/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/out.h>
#include <fsm/options.h>
#include <fsm/bool.h>

#include "internal.h"
#include "walk2.h"

struct fsm *
fsm_intersect(struct fsm *a, struct fsm *b)
{
	struct fsm *q;

	assert(a != NULL);
	assert(b != NULL);

	if (a->opt != b->opt) {
		errno = EINVAL;
		return NULL;
	}

	/*
	 * This is intersection implemented in terms of complements and union,
	 * by DeMorgan's theorem:
	 *
	 *     a \n b = ~(~a \u ~b)
	 *
	 * This could also be done by walking sets of states through both FSM
	 * simultaneously, as described by Hopcroft, Motwani and Ullman
	 * (2001, 2nd ed.) 4.2, Closure under Intersection. However this way
	 * is simpler to implement.
	 */

	if (!fsm_complement(a)) {
		return NULL;
	}

	if (!fsm_complement(b)) {
		return NULL;
	}

	q = fsm_union(a, b);
	if (q == NULL) {
		return NULL;
	}

	if (!fsm_complement(q)) {
		goto error;
	}

	return q;

error:

	fsm_free(q);

	return NULL;
}
