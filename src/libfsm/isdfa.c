/* $Id$ */

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <fsm/fsm.h>

#include "internal.h"
#include "trans.h"

static int
hasduplicateedge(const struct fsm_state *state, int e, const struct fsm_edge *edge)
{
	int i;
	int count;

	/* TODO: assertions! */

	count = 0;
	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		if (state->edges[i].trans == NULL) {
			continue;
		}

		if (i != e) {
			continue;
		}

		count += trans_equal(edge->trans, state->edges[i].trans);
	}

	return count > 1;
}

static int
isdfastate(const struct fsm_state *state)
{
	int i;

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		if (state->edges[i].trans == NULL) {
			continue;
		}

		/* DFA may not have epsilon edges */
		if (state->edges[i].trans->type == FSM_EDGE_EPSILON) {
			return 0;
		}

		/* DFA may not have duplicate edges */
		if (hasduplicateedge(state, i, &state->edges[i])) {
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

