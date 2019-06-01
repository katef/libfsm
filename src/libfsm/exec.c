/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <adt/set.h>
#include <adt/stateset.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include "internal.h"

static struct fsm_state *
nextstate(const struct fsm_state *state, int c)
{
	struct fsm_edge *e;

	assert(state != NULL);

	if (!(e = fsm_hasedge_literal(state, c))) {
		return NULL;
	}

	return state_set_only(e->sl);
}

struct fsm_state *
fsm_exec(const struct fsm *fsm,
	int (*fsm_getc)(void *opaque), void *opaque)
{
	struct fsm_state *state;
	int c;

	assert(fsm != NULL);
	assert(fsm_getc != NULL);

	/* TODO: check prerequisites; that it has literal edges, DFA, etc */

	/* TODO: pass struct of callbacks to call during each event; transitions etc */

	if (!fsm_all(fsm, fsm_isdfa)) {
		errno = EINVAL;
		return NULL;
	}

	state = fsm_getstart(fsm);
	assert(state != NULL);

	while (c = fsm_getc(opaque), c != EOF) {
		state = nextstate(state, c);
		if (state == NULL) {
			return NULL;
		}
	}

	if (!fsm_isend(fsm, state)) {
		return NULL;
	}

	return state;
}

