/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <fsm/fsm.h>

#include <adt/internedstateset.h>

static interned_state_set_id
add_states_from_array(struct interned_state_set_pool *issp,
    size_t length, const fsm_state_t *states, unsigned verbosity)
{
	(void)verbosity;
	interned_state_set_id res;
	if (!interned_state_set_intern_set(issp, length, states, &res)) {
		fprintf(stderr, "interned_state_set_intern_set failed, exiting\n");
		exit(EXIT_FAILURE);
	}

	return res;
}

#define DEF_VERBOSITY 0
static unsigned
get_verbosity(void)
{
	const char *v = getenv("VERBOSITY");
	return (v == NULL ? DEF_VERBOSITY : atoi(v));
}

#define DEF_LIMIT 100
static unsigned
get_limit(void)
{
	const char *v = getenv("LIMIT");
	return (v == NULL ? DEF_LIMIT : atoi(v));
}

int main(void) {
	interned_state_set_id set_up, set_up2;
	fsm_state_t i;
	const size_t limit = get_limit();
	fsm_state_t *states;
	const unsigned verbosity = get_verbosity();

	struct interned_state_set_pool *issp = interned_state_set_pool_alloc(NULL);
	assert(issp);

	states = malloc(2*limit * sizeof(states[0]));
	assert(states != NULL);

	for (i = 0; i < limit; i++) {
		states[i] = i;
	}

	/* Create state sets with [], [0], [0, 1], ... [0, ... limit-1] */
	set_up = add_states_from_array(issp, limit, states, verbosity);

	/* Create the same ones again */
	set_up2 = add_states_from_array(issp, limit, states, verbosity);

	/* Check that they match. */
	assert(set_up == set_up2);

	interned_state_set_pool_free(issp);
	free(states);
	return 0;
}
