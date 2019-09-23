/*
 * Copyright 2019 Shannon Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <adt/set.h>

int cmp_int(const void *a_, const void *b_) {
	int a = *(const int *)a_, b = *(const int *)b_;
	if (a > b)      return 1;
	else if (a < b) return -1;
	else            return 0;
}

/* tests bulk addition */
int main(void) {
	static int items1[3] = {1, 2, 3};
	static int items2[3] = {1, 2, 6};
	static int items3[3] = {-2, -1, 0};
	static int items4[3] = {9, 7, 7};
	static int items5[5] = {-10, 10, -12, 15, 5 };

	static int all_items[] = { -12, -10, -2, -1, 0, 1, 2, 3, 5, 6, 7, 9, 10, 15 };

	struct set *s = set_create(NULL, cmp_int);
	void *ptrs[5] = { &items2[0], &items2[1], &items2[2] };
	size_t i;

	assert(set_add(s, &items1[0]));
	assert(set_add(s, &items1[1]));
	assert(set_add(s, &items1[2]));

	assert(set_count(s) == 3);

	assert(set_contains(s, &items1[0]));
	assert(set_contains(s, &items1[1]));
	assert(set_contains(s, &items1[2]));

	assert(set_add_bulk(s, &ptrs[0], 3));
	assert(set_count(s) == 4);

	assert(set_contains(s, &items2[0]));
	assert(set_contains(s, &items2[1]));
	assert(set_contains(s, &items2[2]));

	ptrs[0] = &items3[0]; ptrs[1] = &items3[1]; ptrs[2] = &items3[2];
	assert(set_add_bulk(s, &ptrs[0], 3));
	assert(set_count(s) == 7);

	assert(set_contains(s, &items3[0]));
	assert(set_contains(s, &items3[1]));
	assert(set_contains(s, &items3[2]));

	ptrs[0] = &items4[0]; ptrs[1] = &items4[1]; ptrs[2] = &items4[2];
	assert(set_add_bulk(s, &ptrs[0], 3));
	assert(set_count(s) == 9);

	assert(set_contains(s, &items4[0]));
	assert(set_contains(s, &items4[1]));
	assert(set_contains(s, &items4[2]));

	ptrs[0] = &items5[0]; ptrs[1] = &items5[1]; ptrs[2] = &items5[2]; ptrs[3] = &items5[3]; ptrs[4] = &items5[4];
	assert(set_add_bulk(s, &ptrs[0], 5));
	assert(set_count(s) == 14);

	assert(set_contains(s, &items5[0]));
	assert(set_contains(s, &items5[1]));
	assert(set_contains(s, &items5[2]));
	assert(set_contains(s, &items5[3]));
	assert(set_contains(s, &items5[4]));

	for (i=0; i < sizeof all_items/sizeof all_items[0]; i++) {
		assert(set_contains(s, &all_items[i]));
	}

	return 0;
}
