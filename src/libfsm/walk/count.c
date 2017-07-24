/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/walk.h>

#include "../internal.h"

unsigned
fsm_count(const struct fsm *fsm, void *pred_arg,
	int (*predicate)(void *, const struct fsm *, const struct fsm_state *))
{
	const struct fsm_state *s;
	unsigned n;

	assert(fsm != NULL);
	assert(predicate != NULL);

	n = 0;

	for (s = fsm->sl; s != NULL; s = s->next) {
		n += !!predicate(pred_arg, fsm, s);
	}

	return n;
}

