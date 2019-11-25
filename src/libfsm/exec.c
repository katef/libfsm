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

#include "internal.h"

static int
nextstate(const struct fsm *fsm, fsm_state_t state, int c,
	fsm_state_t *next)
{
	struct fsm_edge *e;

	assert(state < fsm->statecount);
	assert(next != NULL);

	e = fsm_hasedge_literal(fsm, state, c);
	if (e == NULL) {
		return 0;
	}

	*next = state_set_only(e->sl);
	return 1;
}

int
fsm_exec(const struct fsm *fsm,
	int (*fsm_getc)(void *opaque), void *opaque,
	fsm_state_t *end)
{
	fsm_state_t state;
	int c;

	assert(fsm != NULL);
	assert(fsm_getc != NULL);
	assert(end != NULL);

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
		if (!nextstate(fsm, state, c, &state)) {
			return 0;
		}
	}

	if (!fsm_isend(fsm, state)) {
		return 0;
	}

	*end = state;
	return 1;
}

