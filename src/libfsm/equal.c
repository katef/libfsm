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

#include <adt/set.h>

#include "internal.h"
#include "walk2.h"

static int
ensure_dfa(const struct fsm *fsm, struct fsm **dfa)
{
	struct fsm *q;

	assert(fsm != NULL);
	assert(dfa != NULL);

	if (fsm_all(fsm, fsm_isdfa)) {
		*dfa = NULL;
		return 1;
	}

	q = fsm_clone(fsm);
	if (q == NULL) {
		return 0;
	}

	if (!fsm_determinise(q)) {
		fsm_free(q);
		return 0;
	}

	*dfa = q;

	return 1;
}

static struct fsm *
subtract(const struct fsm *a, const struct fsm *b)
{
	struct fsm *pa, *pb;
	struct fsm *q;

	assert(a != NULL);
	assert(b != NULL);
	assert(a->opt == b->opt);

	if (!ensure_dfa(a, &pa)) {
		return NULL;
	}

	if (pa != NULL) {
		a = pa;
	}

	if (!ensure_dfa(b, &pb)) {
		if (pa != NULL) {
			fsm_free(pa);
		}
		return NULL;
	}

	if (pb != NULL) {
		b = pb;
	}

	/* see fsm_subtract() */
	q = fsm_walk2(a, b, FSM_WALK2_ONLYA | FSM_WALK2_BOTH, FSM_WALK2_ONLYA);

	if (pa != NULL) {
		fsm_free(pa);
	}

	if (pb != NULL) {
		fsm_free(pb);
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

	if (fsm_empty(a) && fsm_empty(b)) {
		return 1;
	}

	/*
	 * The subset operation is not commutative; sets are equal
	 * when one set is a subset of the other and vice versa.
	 * This is equivalent finding that (a \ b) \u (b \ a) is empty.
	 */

	return subsetof(a, b) && subsetof(b, a);
}

