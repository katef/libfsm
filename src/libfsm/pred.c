/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>

#include "internal.h"

static struct fsm_pred_cache *
pred_cache(const struct fsm_state *state)
{
	assert(state != NULL);

	/*
	 * This a const-discarding hack, much like the way strchr() does not modify
	 * a string, but returns a pointer that may be modified. In our case here,
	 * the predicate functions will not modify the structure of an fsm, but do
	 * flip bits in the predicate cache to save their work.
	 *
	 * This is a performance optimisation and always has no effect on the
	 * semantic effect of operations.
	 */

	return (void *) &state->pred_cache;
}

void
pred_set(struct fsm_state *state, enum fsm_pred pred)
{
	assert(state != NULL);
	assert(pred != 0U);

	pred_cache(state)->values |= pred;
}

void
pred_unset(struct fsm_state *state, enum fsm_pred pred)
{
	assert(state != NULL);
	assert(pred != 0U);

	pred_cache(state)->values &= ~ (unsigned) pred;
}

int
pred_get(const struct fsm_state *state, enum fsm_pred pred)
{
	assert(state != NULL);
	assert(pred != 0U);

	return pred_cache(state)->values & pred;
}

