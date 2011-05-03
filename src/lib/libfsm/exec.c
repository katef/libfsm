/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/exec.h>
#include <fsm/graph.h>

#include "internal.h"
#include "set.h"

/* TODO: centralise? */
static int
indexof(const struct fsm *fsm, const struct fsm_state *state)
{
	struct fsm_state *s;
	int i;

	assert(fsm != NULL);
	assert(state != NULL);

	for (s = fsm->sl, i = 0; s != NULL; s = s->next, i++) {
		if (s == state) {
			return i;
		}
	}

	return -1;
}

struct fsm_state *nextstate(const struct fsm_state *state, char c) {
	const struct state_set *s;

	assert(state != NULL);

	s = state->edges[(unsigned char) c].sl;
	if (s == NULL) {
		return NULL;
	}

	assert(s->next  != NULL);
	assert(s->state != NULL);

	return s->state;
}

int fsm_exec(const struct fsm *fsm, int (*fsm_getc)(void *opaque), void *opaque) {
	struct fsm_state *state;
	int c;

	assert(fsm != NULL);
	assert(fsm_getc != NULL);

	/* TODO: check prerequisites; that it has literal edges, DFA, etc */

	/* TODO: pass struct of callbacks to call during each event; transitions etc */

	assert(fsm_isdfa(fsm));
	assert(fsm->start != NULL);
	state = fsm->start;

	while ((c = fsm_getc(opaque)) != EOF) {
		state = nextstate(state, c);
		if (state == NULL) {
			return 0;
		}
	}

	if (!fsm_isend(fsm, state)) {
		return 0;
	}

	return indexof(fsm, state);
}

int fsm_sgetc(void *opaque) {
	const char **s = opaque;
	char c;

	assert(s != NULL);

	c = **s;

	if (c == '\0') {
		return EOF;
	}

	(*s)++;

	return c;
}

