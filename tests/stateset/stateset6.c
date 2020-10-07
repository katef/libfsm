/*
 * Copyright 2017 Ed Kellett
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include <adt/stateset.h>

static void test(fsm_state_t n) {
	struct state_set *s = NULL;
	struct state_iter iter;
	fsm_state_t i;
	fsm_state_t p;

	p = 999;
	for (i = 0; i < n; i++) {
		assert(state_set_add(&s, NULL, i));
	}
	for (state_set_reset(s, &iter), i = 0; state_set_next(&iter, &p); i++) {
		assert(p == i);
	}
	assert(i == n);
	assert(p == (n == 0 ? 999 : n - 1));
	state_set_free(s);
}

int main(void) {
	test(0);
	test(1);
	test(2);
	test(3);
	test(100);
	test(1000);
	test(5000);
	test(50000);

	return 0;
}
