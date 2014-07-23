/* $Id$ */

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <errno.h>

#include <adt/priq.h> /* XXX: path adt */

#include <fsm/fsm.h>
#include <fsm/cost.h>
#include <fsm/search.h>

int
fsm_example(struct fsm *fsm, const struct fsm_state *goal,
	char *buf, size_t bufsz)
{
	const struct fsm_state *start;
	const struct priq *p;
	struct priq *path;
	size_t n;

	assert(fsm != NULL);
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

	/* TODO: possibly make "path" ADT, which has states and edges */
	path = fsm_shortest(fsm, fsm_getstart(fsm), goal, fsm_cost_legible);
	if (path == NULL) {
		return -1;
	}

	for (p = priq_find(path, goal), n = 0; p != NULL; p = p->prev, n++) {
		if (bufsz > 0) {
			*buf++ = p->prev ? p->type : '\0';
			bufsz--;
		}
	}

	priq_free(path);

	return n;
}

