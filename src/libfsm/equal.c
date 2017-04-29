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

struct fsm *
subtract(const struct fsm *a, const struct fsm *b)
{
	struct fsm *q;

	assert(a != NULL);
	assert(b != NULL);

	a = fsm_clone(a);
	if (a == NULL) {
		return NULL;
	}

	b = fsm_clone(b);
	if (b == NULL) {
		fsm_free(a);
		return NULL;
	}

	q = fsm_subtract(a, b);
	if (q == NULL) {
		fsm_free(a);
		fsm_free(b);
		return NULL;
	}

	return q;
}

static int
subsetof(const struct fsm *a, const struct fsm *b)
{
	struct fsm *q;
	int r;

	assert(a != NULL);
	assert(b != NULL);

	q = subtract(b, a);
	if (q == NULL) {
		return -1;
	}

	r = fsm_empty(q);

	fsm_free(q);

	if (r == -1) {
		return -1;
	}

	return r;
}

int
fsm_equal(const struct fsm *a, const struct fsm *b)
{
	assert(a != NULL);
	assert(b != NULL);

	if (a->opt != b->opt) {
		errno = EINVAL;
		return -1;
	}

	/*
	 * The subset operation is not commutative; sets are equal
	 * when one set is a subset of the other and vice versa.
	 * This is equivalent finding that (a \ b) \u (b \ a) is empty.
	 */

	return subsetof(a, b) && subsetof(b, a);
}

