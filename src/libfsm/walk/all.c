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

int
fsm_all(const struct fsm *fsm, void *pred_arg,
	int (*predicate)(void *, const struct fsm *, const struct fsm_state *))
{
	const struct fsm_state *s;

	assert(fsm != NULL);
	assert(predicate != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		if (!predicate(pred_arg, fsm, s)) {
			return 0;
		}
	}

	return 1;
}

