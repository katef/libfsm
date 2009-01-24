/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/exec.h>

#include "internal.h"

static struct fsm_state *nextstate(const struct fsm_edge *edges, char c) {
	const struct fsm_edge *e;

	for (e = edges; e; e = e->next) {
		assert(e->trans->type == FSM_EDGE_LITERAL);

		if (e->trans->u.literal == c) {
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

	return state->end;
}

