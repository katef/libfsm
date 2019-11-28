/*
 * Copyright 2017 Ed Kellett
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include <adt/stateset.h>

fsm_state_t next_state(void) {
	static fsm_state_t n = 0;
	return n++;
}

int main(void) {
	struct state_set *s = NULL;
	fsm_state_t a[3] = {1200,2400,3600};
	size_t i;
	for (i = 0; i < 5000; i++) {
		assert(state_set_add(&s, NULL, next_state()));
	}
	for (i = 0; i < 3; i++) {
		assert(state_set_contains(s, a[i]));
		state_set_remove(&s, a[i]);
	}
	assert(state_set_count(s) == 4997);
	return 0;
}
