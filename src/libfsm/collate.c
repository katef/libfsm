/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/walk.h>

#include "internal.h"

int
fsm_collate(struct fsm *fsm, fsm_state_t *state,
	int (*predicate)(const struct fsm *, fsm_state_t))
{
	assert(fsm != NULL);
	assert(state != NULL);
	assert(predicate != NULL);

	switch (fsm_count(fsm, predicate)) {
		fsm_state_t new, i;

	case 0:
		errno = EINVAL; /* bad form */
		return 0;

	case 1:
		for (i = 0; !predicate(fsm, i); i++)
			;

		*state = i;
		return 1;

	default:
		if (!fsm_addstate(fsm, &new)) {
			return 0;
		}

		for (i = 0; i < fsm->statecount; i++) {
			if (i == new) {
				continue;
			}

			if (!predicate(fsm, i)) {
				continue;
			}

			if (!fsm_addedge_epsilon(fsm, i, new)) {
				/* TODO: free new */
				return 0;
			}
		}

		*state = new;
		return 1;
	}
}

