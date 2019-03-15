/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>

#include "internal.h"

void
pred_set(struct fsm_state *state, enum fsm_pred pred)
{
	assert(state != NULL);
	assert(pred != 0U);

	state->pred |= pred;
}

void
pred_unset(struct fsm_state *state, enum fsm_pred pred)
{
	assert(state != NULL);
	assert(pred != 0U);

	state->pred &= ~ (unsigned) pred;
}

int
pred_get(const struct fsm_state *state, enum fsm_pred pred)
{
	assert(state != NULL);
	assert(pred != 0U);

	return state->pred & pred;
}

