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
#include <fsm/capture.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "internal.h"
#include "capture.h"

#define LOG_EXEC 0

static int
transition(const struct fsm *fsm, fsm_state_t state, int c,
	size_t offset, struct fsm_capture *captures,
	fsm_state_t *next)
{
	assert(state < fsm->statecount);
	assert(next != NULL);

	if (!edge_set_transition(fsm->states[state].edges, c, next)) {
		return 0;
	}

	if (captures != NULL && fsm_capture_has_capture_actions(fsm, state)) {
		fsm_capture_update_captures(fsm, state, *next,
		    offset, captures);
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
	size_t offset = 0;
	unsigned i;
	size_t capture_count;

	assert(fsm != NULL);
	assert(fsm_getc != NULL);
	assert(end != NULL);

	capture_count = fsm_countcaptures(fsm);

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

	for (i = 0; i < capture_count; i++) {
		captures[i].pos[0] = FSM_CAPTURE_NO_POS;
		captures[i].pos[1] = FSM_CAPTURE_NO_POS;
	}

#if LOG_EXEC
	fprintf(stderr, "fsm_exec: starting at %d\n", state);
#endif

	while (c = fsm_getc(opaque), c != EOF) {
		if (!transition(fsm, state, c, offset, captures, &state)) {
#if LOG_EXEC
			fprintf(stderr, "fsm_exec: edge not found\n");
#endif
			return 0;
		}

#if LOG_EXEC
		fprintf(stderr, "fsm_exec: @ %zu, input '%c', new state %u\n",
		    offset, c, state);
#endif
		offset++;
	}

	if (!fsm_isend(fsm, state)) {
		return 0;
	}

	/* Check for capture actions on end state */
	if (captures != NULL && fsm_capture_has_capture_actions(fsm, state)) {
		fsm_capture_update_captures(fsm, state, NEXT_STATE_END,
		    offset, captures);
	}

	fsm_capture_finalize_captures(fsm, capture_count, captures);

	*end = state;
	return 1;
}

