/* $Id$ */

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <fsm/fsm.h>

#include "internal.h"

static int
edgecount(const struct fsm_state *state, const char *label)
{
	int count;
	const struct fsm_edge *e;

	count = 0;
	for (e = state->edges; e; e = e->next) {
		count += 0 == strcmp(e->label->label, label);
	}

	return count;
}

static int
isdfastate(const struct fsm_state *state)
{
	const struct fsm_edge *e;

	for (e = state->edges; e; e = e->next) {
		/* DFA may not have epsilon edges */
		if (e->label->label == NULL) {
			return 0;
		}

		/* DFA may not have duplicate edges */
		if (edgecount(state, e->label->label) > 1) {
			return 0;
		}
	}

	return 1;
}

int
fsm_isdfa(const struct fsm *fsm)
{
	const struct state_list *s;

	assert(fsm != NULL);
	assert(fsm->start != NULL);

	if (fsm->sl == NULL) {
		return 0;
	}

	for (s = fsm->sl; s; s = s->next) {
		if (!isdfastate(&s->state)) {
			return 0;
		}
	}

	return 1;
}

