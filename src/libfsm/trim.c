/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <limits.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include "internal.h"

int
fsm_trim(struct fsm *fsm)
{
	struct fsm_state *s;
	struct fsm_state *next;
	int r;

	assert(fsm != NULL);

	do {
		r = 0;

		for (s = fsm->sl; s != NULL; s = next) {
			next = s->next;

			if (fsm_isend(fsm, s)) {
				continue;
			}

			if (!fsm_hasoutgoing(fsm, s)) {
				fsm_removestate(fsm, s);
				r = 1;
			}
		}
	} while (r);

	return 1;
}

