/*
 * Copyright 2019 Shannon Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <adt/set.h>

typedef int item_t;

#include "set.inc"

static int
cmp_int(const void *a, const void *b)
{
	const int *pa = * (const int * const *) a;
	const int *pb = * (const int * const *) b;

	if (*pa > *pb)      return +1;
	else if (*pa < *pb) return -1;
	else                return  0;
}

/* tests bulk addition */
int main(void) {
	size_t i;
	struct set *s;

	static int values1[3] = {1, 2, 3};
	static int values2[3] = {1, 2, 6};
	static int values3[3] = {-2, -1, 0};
	static int values4[3] = {9, 7, 7};
	static int values5[5] = {-10, 10, -12, 15, 5 };

	static int all_values[] = { -12, -10, -2, -1, 0, 1, 2, 3, 5, 6, 7, 9, 10, 15 };

	static int *items1[3] = {&values1[0], &values1[1], &values1[2]};
	static int *items2[3] = {&values2[0], &values2[1], &values2[2]};
	static int *items3[3] = {&values3[0], &values3[1], &values3[2]};
	static int *items4[3] = {&values4[0], &values4[1], &values4[2]};
	static int *items5[5] = {&values5[0], &values5[1], &values5[2], &values5[3], &values5[4]};

	int *all_items[sizeof all_values / sizeof all_values[0]];

	for(i=0; i < sizeof all_items/sizeof all_items[0]; i++) {
		all_items[i] = &all_values[i];
	}

	s = set_create(NULL, cmp_int);

	assert(set_add(s, items1[0]));
	assert(set_add(s, items1[1]));
	assert(set_add(s, items1[2]));

	assert(set_count(s) == 3);

	assert(set_contains(s, items1[0]));
	assert(set_contains(s, items1[1]));
	assert(set_contains(s, items1[2]));

	assert(set_add_bulk(s, &items2[0], 3));
	assert(set_count(s) == 4);

	assert(set_contains(s, items2[0]));
	assert(set_contains(s, items2[1]));
	assert(set_contains(s, items2[2]));

	assert(set_add_bulk(s, &items3[0], 3));
	assert(set_count(s) == 7);

	assert(set_contains(s, items3[0]));
	assert(set_contains(s, items3[1]));
	assert(set_contains(s, items3[2]));

	assert(set_add_bulk(s, &items4[0], 3));
	assert(set_count(s) == 9);

	assert(set_contains(s, items4[0]));
	assert(set_contains(s, items4[1]));
	assert(set_contains(s, items4[2]));

	assert(set_add_bulk(s, &items5[0], 5));
	assert(set_count(s) == 14);

	assert(set_contains(s, items5[0]));
	assert(set_contains(s, items5[1]));
	assert(set_contains(s, items5[2]));
	assert(set_contains(s, items5[3]));
	assert(set_contains(s, items5[4]));

	for (i=0; i < sizeof all_items/sizeof all_items[0]; i++) {
		assert(set_contains(s, all_items[i]));
	}

	return 0;
}
