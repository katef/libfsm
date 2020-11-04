/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "internal.h"

static int
transition(const struct fsm *fsm, fsm_state_t state, int c,
	fsm_state_t *next)
{
	assert(state < fsm->statecount);
	assert(next != NULL);

	if (!edge_set_transition(fsm->states[state].edges, c, next)) {
		return 0;
	}

	return 1;
}

int
fsm_exec(const struct fsm *fsm,
	int (*fsm_getc)(void *opaque), void *opaque,
	fsm_state_t *end, struct fsm_capture *captures)
{
	fsm_state_t state;
	int c;

	assert(fsm != NULL);
	assert(fsm_getc != NULL);
	assert(end != NULL);

	(void)captures;

	/* TODO: check prerequisites; that it has literal edges, DFA, etc */

	/* TODO: pass struct of callbacks to call during each event; transitions etc */

	if (!fsm_all(fsm, fsm_isdfa)) {
		errno = EINVAL;
		return -1;
	}

	if (!fsm_getstart(fsm, &state)) {
		errno = EINVAL;
		return -1;
	}

	while (c = fsm_getc(opaque), c != EOF) {
		if (!transition(fsm, state, c, &state)) {
			return 0;
		}
	}

	if (!fsm_isend(fsm, state)) {
		return 0;
	}

	*end = state;
	return 1;
}

