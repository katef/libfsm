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
	size_t i;
	for (i = 0; i < 5000; i++) {
		assert(state_set_add(&s, NULL, next_state()));
	}
	assert(state_set_count(s) == 5000);
	return 0;
}
