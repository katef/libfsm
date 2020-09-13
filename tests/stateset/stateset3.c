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
	struct state_iter iter;
	fsm_state_t p;
	fsm_state_t a[3] = {1, 2, 3};
	int seen[3] = {0, 0, 0};
	size_t i;
	assert(state_set_add(&s, NULL, a[0]));
	assert(state_set_add(&s, NULL, a[1]));
	assert(state_set_add(&s, NULL, a[2]));
	for (state_set_reset(s, &iter); state_set_next(&iter, &p); ) {
		assert(p == 1 || p == 2 || p == 3);
		seen[p - 1] = 1;
	}
	for (i = 0; i < 3; i++) {
		assert(seen[i]);
	}
	state_set_free(s);
	return 0;
}
