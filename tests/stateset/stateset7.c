/*
 * Copyright 2019 Shannon Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include <adt/stateset.h>

/* tests bulk addition */
int main(void) {
	size_t i;
	struct state_set *s;

	static fsm_state_t values1[3] = {1, 2, 3};
	static fsm_state_t values2[3] = {1, 2, 6};
	static fsm_state_t values3[3] = {-2, -1, 0};
	static fsm_state_t values4[3] = {9, 7, 7};
	static fsm_state_t values5[5] = {-10, 10, -12, 15, 5 };

	static fsm_state_t all_values[] = { -12, -10, -2, -1, 0, 1, 2, 3, 5, 6, 7, 9, 10, 15 };

	static fsm_state_t *items1[3] = {&values1[0], &values1[1], &values1[2]};
	static fsm_state_t *items2[3] = {&values2[0], &values2[1], &values2[2]};
	static fsm_state_t *items3[3] = {&values3[0], &values3[1], &values3[2]};
	static fsm_state_t *items4[3] = {&values4[0], &values4[1], &values4[2]};
	static fsm_state_t *items5[5] = {&values5[0], &values5[1], &values5[2], &values5[3], &values5[4]};

	fsm_state_t *all_items[sizeof all_values / sizeof all_values[0]];

	for(i=0; i < sizeof all_items/sizeof all_items[0]; i++) {
		all_items[i] = &all_values[i];
	}

	s = NULL;

	assert(state_set_add(&s, NULL, *items1[0]));
	assert(state_set_add(&s, NULL, *items1[1]));
	assert(state_set_add(&s, NULL, *items1[2]));

	assert(state_set_count(s) == 3);

	assert(state_set_contains(s, *items1[0]));
	assert(state_set_contains(s, *items1[1]));
	assert(state_set_contains(s, *items1[2]));

	assert(state_set_add_bulk(&s, NULL, items2[0], 3));
	assert(state_set_count(s) == 4);

	assert(state_set_contains(s, *items2[0]));
	assert(state_set_contains(s, *items2[1]));
	assert(state_set_contains(s, *items2[2]));

	assert(state_set_add_bulk(&s, NULL, items3[0], 3));
	assert(state_set_count(s) == 7);

	assert(state_set_contains(s, *items3[0]));
	assert(state_set_contains(s, *items3[1]));
	assert(state_set_contains(s, *items3[2]));

	assert(state_set_add_bulk(&s, NULL, items4[0], 3));
	assert(state_set_count(s) == 9);

	assert(state_set_contains(s, *items4[0]));
	assert(state_set_contains(s, *items4[1]));
	assert(state_set_contains(s, *items4[2]));

	assert(state_set_add_bulk(&s, NULL, items5[0], 5));
	assert(state_set_count(s) == 14);

	assert(state_set_contains(s, *items5[0]));
	assert(state_set_contains(s, *items5[1]));
	assert(state_set_contains(s, *items5[2]));
	assert(state_set_contains(s, *items5[3]));
	assert(state_set_contains(s, *items5[4]));

	for (i=0; i < sizeof all_items/sizeof all_items[0]; i++) {
		assert(state_set_contains(s, *all_items[i]));
	}

	state_set_free(s);
	return 0;
}
