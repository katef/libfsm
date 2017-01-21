/* $Id$ */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include "internal.h"

static struct fsm_state *
nextstate(const struct fsm_state *state, char c)
{
	const struct set *s;
	struct fsm_state *ret;
	struct set_iter it;

	assert(state != NULL);

	s = state->edges[(unsigned char) c].sl;
	if (s == NULL) {
		return NULL;
	}

	ret = set_first(s, &it);

	assert(ret != NULL);

	return ret;
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

	assert(fsm->start != NULL);
	state = fsm->start;

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

