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
	fsm_state_t a = 1;
	assert(state_set_add(&s, NULL, a));
	assert(state_set_add(&s, NULL, a));
	assert(state_set_add(&s, NULL, a));
	state_set_remove(&s, a);
	assert(!state_set_contains(s, a));
	return 0;
}
