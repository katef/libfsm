/*
 * Copyright 2017 Ed Kellett
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include <adt/stateset.h>

int main(void) {
	struct state_set *s = NULL;
	fsm_state_t a[3] = {1, 2, 3};

	assert(!state_set_contains(s, a[0]));
	assert(!state_set_contains(s, a[1]));
	assert(!state_set_contains(s, a[1]));

	assert(state_set_add(&s, NULL, a[0]));
	assert(state_set_contains(s, a[0]));
	assert(!state_set_contains(s, a[1]));
	assert(!state_set_contains(s, a[1]));

	assert(state_set_add(&s, NULL, a[1]));
	assert(state_set_contains(s, a[0]));
	assert(state_set_contains(s, a[1]));
	assert(!state_set_contains(s, a[2]));

	assert(state_set_add(&s, NULL, a[2]));
	assert(state_set_contains(s, a[0]));
	assert(state_set_contains(s, a[1]));
	assert(state_set_contains(s, a[2]));

	return 0;
}
