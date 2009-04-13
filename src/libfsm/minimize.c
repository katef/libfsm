/* $Id$ */

#include <assert.h>
#include <stddef.h>

#include <fsm/graph.h>

int
fsm_minimize(struct fsm *fsm)
{
	int r;

	assert(fsm != NULL);

	/*
	 * Brzozowski's algorithm.
	 */

	r = fsm_reverse(fsm);
	if (r <= 0) {
		return r;
	}

	r = fsm_todfa(fsm);
	if (r <= 0) {
		return r;
	}

	r = fsm_reverse(fsm);
	if (r <= 0) {
		return r;
	}

	r = fsm_todfa(fsm);
	if (r <= 0) {
		return r;
	}

	return 1;
}

