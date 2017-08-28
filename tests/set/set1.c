/*
 * Copyright 2017 Ed Kellett
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

int main(void) {
	struct set *s = set_create(cmp_int);
	int a[3] = {1, 2, 3};
	assert(set_add(&s, &a[0]));
	assert(set_add(&s, &a[1]));
	assert(set_add(&s, &a[2]));
	assert(set_contains(s, &a[0]));
	assert(set_contains(s, &a[1]));
	assert(set_contains(s, &a[2]));
	return 0;
}
