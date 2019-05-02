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

struct fsm_state *
fsm_collate(struct fsm *fsm,
	int (*predicate)(const struct fsm *, const struct fsm_state *))
{
	struct fsm_state *new;
	size_t i;

	assert(fsm != NULL);
	assert(predicate != NULL);

	switch (fsm_count(fsm, predicate)) {
	case 0:
		errno = 0; /* XXX: bad form */
		return NULL;

	case 1:
		for (i = 0; !predicate(fsm, fsm->states[i]); i++)
			;

		return fsm->states[i];

	default:
		new = fsm_addstate(fsm);
		if (new == NULL) {
			return NULL;
		}

		for (i = 0; i < fsm->statecount; i++) {
			if (fsm->states[i] == new) {
				continue;
			}

			if (!predicate(fsm, fsm->states[i])) {
				continue;
			}

			if (!fsm_addedge_epsilon(fsm, fsm->states[i], new)) {
				/* TODO: free new */
				return NULL;
			}
		}

		return new;
	}
}

