/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/exec.h>
#include <fsm/graph.h>

#include "internal.h"

static struct fsm_state *nextstate(const struct fsm_edge edges[], char c) {
	int i;

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		switch (edges[i].trans->type) {
		case FSM_EDGE_LITERAL:
			if (i == c) {
				return edges[i].state;
			}
			continue;

		case FSM_EDGE_EPSILON:
			assert(!"unreached");
			return NULL;
		}
	}

	return NULL;
}

int fsm_exec(const struct fsm *fsm, int (*fsm_getc)(void *opaque), void *opaque) {
	struct fsm_state *state;
	int c;

	assert(fsm != NULL);
	assert(fsm_getc != NULL);

	/* TODO: check prerequisites; that it has literal edges (or any), DFA, etc */

	/* TODO: pass struct of callbacks to call during each event; transitions etc */

	assert(fsm_isdfa(fsm));
	assert(fsm->start != NULL);
	state = fsm->start;

	while ((c = fsm_getc(opaque)) != EOF) {
		state = nextstate(state->edges, c);
		if (state == NULL) {
			return 0;
		}
	}

	if (!state->end) {
		return 0;
	}

	return state->id;
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

