/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/exec.h>

#include "internal.h"

static struct fsm_state *nextstate(const struct fsm_edge *edges, int c) {
	char label[2];
	const struct fsm_edge *e;

	label[0] = c;
	label[1] = '\0';

	for (e = edges; e; e = e->next) {
		assert(e->label->label != NULL);
		assert(strlen(e->label->label) == 1);

		if (0 == strcmp(e->label->label, label)) {
			return e->state;
		}
	}

	return NULL;
}

int fsm_exec(const struct fsm *fsm, int (*fsm_getc)(void *opaque), void *opaque) {
	struct fsm_state *state;
	int c;

	assert(fsm != NULL);
	assert(fsm_getc != NULL);

	/* TODO: assert that the given FSM is a DFA */

	/* TODO: pass struct of callbacks to call during each event; transitions etc */

	assert(fsm->start != NULL);
	state = fsm->start;

	while ((c = fsm_getc(opaque)) != EOF) {
		state = nextstate(state->edges, c);
		if (state == NULL) {
			return 0;
		}
	}

	return fsm_isend(fsm, state);
}

