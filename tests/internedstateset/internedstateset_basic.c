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

static void
reverse(size_t length, fsm_state_t *states)
{
	size_t i;
	for (i = 0; i < length/2; i++) {
		const size_t rev_i = length - i - 1;
		const fsm_state_t tmp = states[rev_i];
		states[rev_i] = states[i];
		states[i] = tmp;
	}
}

static void
fisher_yates_shuffle(size_t length, fsm_state_t *states, unsigned seed)
{
	size_t i;
	srandom(seed);
	assert(length > 1);
	for (i = 0; i < length - 1; i++) {
		/* pick j: i <= j < n */
		const size_t j = i + (random() % (length - i));

		const fsm_state_t tmp = states[i];
		assert(j < length);
		states[i] = states[j];
		states[j] = tmp;
	}
}

static interned_state_set_id
add_states_from_array(struct interned_state_set_pool *issp,
    size_t length, const fsm_state_t *states, unsigned verbosity)
{
	size_t i;
	interned_state_set_id empty = interned_state_set_empty(issp);
	interned_state_set_id cur, prev;

	cur = empty;

	for (i = 0; i < length; i++) {
		interned_state_set_id next;
		const fsm_state_t s = states[i];
		if (verbosity > 1) {
			fprintf(stderr, "-- %zu: adding %d\n", i, s);
		}
		prev = cur;

		/* This is handing off the older reference. */
		if (!interned_state_set_add(issp, &cur, s, &next)) {
			fprintf(stderr, "interned_state_set_add failed, exiting\n");
			exit(EXIT_FAILURE);
		}
		cur = next;
	}

	if (verbosity > 0) {
		fprintf(stderr, "== %s: done\n", __func__);
		interned_state_set_dump(issp, cur);
	}

	return cur;
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

#define DEF_SHUFFLE_CYCLES 10
static unsigned
get_shuffle_cycles(void)
{
	const char *v = getenv("SHUFFLE");
	return (v == NULL ? DEF_SHUFFLE_CYCLES : atoi(v));
}

int main(void) {
	interned_state_set_id set_up, set_up2, set_down, set_down2, doubled;
	fsm_state_t i;
	const size_t limit = get_limit();
	const size_t shuffle_cycles = get_shuffle_cycles();
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
	interned_state_set_release(issp, &set_up2);

	/* Create state sets [], [limit-1], [limit-2, limit-1], ... */
	reverse(limit, states);
	set_down = add_states_from_array(issp, limit, states, verbosity);

	/* Both should end up with the same state set once all are added. */
	assert(set_up == set_down);

	/* Create the state set in descending order again. These should all
	 * be cached. */
	set_down2 = add_states_from_array(issp, limit, states, verbosity);

	/* These should also be equal. */
	assert(set_down2 == set_down);
	interned_state_set_release(issp, &set_down);
	interned_state_set_release(issp, &set_down2);

	/* Repeatedly shuffle with different seeds and compare */
	for (i = 0; i < shuffle_cycles; i++) {
		interned_state_set_id shuffled;
		fisher_yates_shuffle(limit, states, i);
		shuffled = add_states_from_array(issp, limit, states, verbosity);
		assert(shuffled == set_up);
		interned_state_set_release(issp, &shuffled);
	}

	/* This should be able to handle duplicated values, so write [0..n]
	 * into the array twice and build up the array with repeated values. */
	for (i = 0; i < limit; i++) {
		states[i + 0] = i;
		states[i + limit] = i;
	}
	doubled = add_states_from_array(issp, 2*limit, states, verbosity);

	assert(doubled == set_up);

	interned_state_set_release(issp, &doubled);
	interned_state_set_release(issp, &set_up);

	interned_state_set_pool_free(issp);
	free(states);
	return 0;
}
