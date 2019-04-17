/*
 * Copyright 2018 Shannon Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <adt/hashset.h>

int cmp_int(const void *a_, const void *b_) {
	int a = *(const int *)a_, b = *(const int *)b_;
	if (a > b)      return 1;
	else if (a < b) return -1;
	else            return 0;
}

unsigned long hash_int(const void *a_)
{
	const int *a = a_;
	return hashrec(a, sizeof *a);
}

int main(void) {
	struct hashset *s = hashset_create(NULL, hash_int,cmp_int);
	struct hashset_iter iter;
	int *p;
	int a[3] = {1, 2, 3};
	int seen[3] = {0, 0, 0};
	int i;
	assert(hashset_add(s, &a[0]));
	assert(hashset_add(s, &a[1]));
	assert(hashset_add(s, &a[2]));

	for (p = hashset_first(s, &iter); p != NULL; p = hashset_next(&iter)) {
		assert(*p == 1 || *p == 2 || *p == 3);
		seen[*p - 1] = 1;
	}

	for (i = 0; i < 3; i++) {
		assert(seen[i]);
	}

	return 0;
}
