/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/cost.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <adt/path.h>

#include "internal.h"

int
fsm_example(const struct fsm *fsm, fsm_state_t goal,
	char *buf, size_t bufsz)
{
	const struct path *p;
	struct path *path;
	fsm_state_t start;
	size_t n;

	assert(fsm != NULL);
	assert(!fsm_has(fsm, fsm_hasepsilons));
	assert(buf != NULL || bufsz == 0);
	assert(goal < fsm->statecount);

	if (bufsz > INT_MAX) {
		errno = EINVAL;
		return -1;
	}

	if (!fsm_getstart(fsm, &start)) {
		errno = EINVAL;
		return -1;
	}

	path = fsm_shortest(fsm, start, goal, fsm_cost_legible);
	if (path == NULL) {
		return -1;
	}

	n = 0;

	if (path->state != start) {
		/* no known path to goal */
		goto done;
	}

	assert(path->c == '\0');

	for (p = path->next; p != NULL; p = p->next) {
		if (bufsz > 1) {
			*buf++ = p->c;
			bufsz--;
		}

		n++;
	}

done:

	if (bufsz > 0) {
		*buf++ = '\0';
		bufsz--;
	}

	path_free(fsm->opt->alloc, path);

	return n;
}

