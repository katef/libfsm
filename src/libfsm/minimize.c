/* $Id$ */

#include <assert.h>
#include <stddef.h>

#include <fsm/graph.h>

struct fsm *
fsm_minimize(struct fsm *fsm)
{
	assert(fsm != NULL);

	/*
	 * Brzozowski's algorithm.
	 */

	if (fsm_reverse(fsm) == NULL) {
		return NULL;
	}

	if (fsm_todfa(fsm)   == NULL) {
		return NULL;
	}

	if (fsm_reverse(fsm) == NULL) {
		return NULL;
	}

	if (fsm_todfa(fsm)   == NULL) {
		return NULL;
	}

	return fsm;
}

