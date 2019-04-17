/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <errno.h>

#include <adt/path.h>

#include <fsm/fsm.h>
#include <fsm/cost.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include "internal.h"

int
fsm_example(const struct fsm *fsm, const struct fsm_state *goal,
	char *buf, size_t bufsz)
{
	const struct fsm_state *start;
	const struct path *p;
	struct path *path;
	size_t n;

	assert(fsm != NULL);
	assert(!fsm_has(fsm, fsm_hasepsilons));
	assert(buf != NULL || bufsz == 0);
	assert(goal != NULL);
	/* TODO: assert goal is in fsm */

	if (bufsz > INT_MAX) {
		errno = EINVAL;
		return -1;
	}

	start = fsm_getstart(fsm);
	if (start == NULL) {
		errno = EINVAL;
		return -1;
	}

	path = fsm_shortest(fsm, fsm_getstart(fsm), goal, fsm_cost_legible);
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

