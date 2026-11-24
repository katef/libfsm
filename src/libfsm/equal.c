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

static int
walk_is_empty(const struct fsm *pa, const struct fsm *pb, unsigned walk_mask)
{
	struct fsm *q;
	int r;

	assert(pa != NULL);
	assert(pb != NULL);

	q = fsm_walk2(pa, pb, walk_mask | FSM_WALK2_BOTH, walk_mask);
	if (q == NULL) {
		return -1;
	}

	r = fsm_empty(q);

	fsm_free(q);

	return r;
}

int
fsm_equal(const struct fsm *a, const struct fsm *b)
{
	struct fsm *pa, *pb;

	assert(a != NULL);
	assert(b != NULL);

	if (a->alloc != b->alloc) {
		errno = EINVAL;
		return -1;
	}

	if (fsm_empty(a) && fsm_empty(b)) {
		return 1;
	}

	if (!ensure_dfa(a, &pa)) {
		return -1;
	}

	if (!ensure_dfa(b, &pb)) {
		fsm_free(pa);
		return -1;
	}

	if (1 != walk_is_empty(pa ? pa : a, pb ? pb : b, FSM_WALK2_ONLYA | FSM_WALK2_ONLYB)) {
		goto unequal;
	}

	if (pa != NULL) {
		fsm_free(pa);
	}
	if (pb != NULL) {
		fsm_free(pb);
	}

	return 1;

unequal:

	if (pa != NULL) {
		fsm_free(pa);
	}
	if (pb != NULL) {
		fsm_free(pb);
	}

	return 0;
}

