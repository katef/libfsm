/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <limits.h>
#include <errno.h>

#include <adt/set.h>

#include <fsm/fsm.h>

#include "internal.h"

int
fsm_equal(const struct fsm *a, const struct fsm *b)
{
	struct fsm *q;
	int r;

	assert(a != NULL);
	assert(b != NULL);

	if (a->opt != b->opt) {
		errno = EINVAL;
		return NULL;
	}

	a = fsm_clone(a);
	if (a == NULL) {
		return -1;
	}

	b = fsm_clone(b);
	if (b == NULL) {
		fsm_free(a);
		return -1;
	}

	q = fsm_subtract(a, b);
	if (q == NULL) {
		return -1;
	}

	r = fsm_empty(q);
	if (r == -1) {
		return -1;
	}

	fsm_free(q);

	return r;
}

