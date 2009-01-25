/* $Id$ */

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <fsm/fsm.h>

#include "internal.h"
#include "trans.h"

static int
hasduplicateedge(const struct fsm_state *state, const struct fsm_edge *edge)
{
	int count;
	const struct fsm_edge *e;

	/* TODO: assertions! */

	count = 0;
	for (e = state->edges; e; e = e->next) {
		assert(e->trans != NULL);

		count += trans_equal(edge->trans, e->trans);
	}

	return count > 1;
}

static int
isdfastate(const struct fsm_state *state)
{
	const struct fsm_edge *e;

	for (e = state->edges; e; e = e->next) {
		assert(e->trans != NULL);

		/* DFA may not have epsilon edges */
		if (e->trans->type == FSM_EDGE_EPSILON) {
			return 0;
		}

		/* DFA may not have duplicate edges */
		if (hasduplicateedge(state, e)) {
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

