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
	struct fsm_state *s;

	assert(fsm != NULL);
	assert(predicate != NULL);

	switch (fsm_count(fsm, predicate)) {
	case 0:
		errno = 0; /* XXX: bad form */
		return NULL;

	case 1:
		for (s = fsm->sl; !predicate(fsm, s); s = s->next) {
			assert(s->next != NULL);
		}

		return s;

	default:
		new = fsm_addstate(fsm);
		if (new == NULL) {
			return NULL;
		}

		for (s = fsm->sl; s != NULL; s = s->next) {
			if (s == new) {
				continue;
			}

			if (!predicate(fsm, s)) {
				continue;
			}

			if (!fsm_addedge_epsilon(fsm, s, new)) {
				/* TODO: free new */
				return NULL;
			}
		}

		return new;
	}
}

