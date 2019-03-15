/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include "internal.h"

void
fsm_setend(struct fsm *fsm, struct fsm_state *state, int end)
{
	(void) fsm;

	assert(fsm != NULL);
	assert(state != NULL);

	if (!!pred_get(state, PRED_ISEND) == !!end) {
		return;
	}

	switch (end) {
	case 0:
		assert(fsm->endcount > 0);
		fsm->endcount--;
		pred_unset(state, PRED_ISEND);
		break;

	case 1:
		assert(fsm->endcount < FSM_ENDCOUNT_MAX);
		fsm->endcount++;
		pred_set(state, PRED_ISEND);
		break;
	}
}

void
fsm_setendopaque(struct fsm *fsm, void *opaque)
{
	struct fsm_state *s;

	assert(fsm != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		if (fsm_isend(fsm, s)) {
			fsm_setopaque(fsm, s, opaque);
		}
	}
}

void
fsm_setopaque(struct fsm *fsm, struct fsm_state *state, void *opaque)
{
	(void) fsm;

	assert(fsm != NULL);
	assert(state != NULL);

	assert(pred_get(state, PRED_ISEND));

	state->opaque = opaque;
}

void *
fsm_getopaque(const struct fsm *fsm, const struct fsm_state *state)
{
	(void) fsm;

	assert(fsm != NULL);
	assert(state != NULL);

	assert(pred_get(state, PRED_ISEND));

	return state->opaque;
}

