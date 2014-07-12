/* $Id$ */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include "internal.h"

unsigned
fsm_count(const struct fsm *fsm,
	int (*predicate)(const struct fsm *, const struct fsm_state *))
{
	const struct fsm_state *s;
	unsigned n;

	assert(fsm != NULL);
	assert(predicate != NULL);

	n = 0;

	for (s = fsm->sl; s != NULL; s = s->next) {
		n += !!predicate(fsm, s);
	}

	return n;
}

