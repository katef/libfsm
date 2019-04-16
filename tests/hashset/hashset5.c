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

int *next_int(void) {
	static int n = 0;
	int *p = malloc(sizeof *p);
	if (p == NULL) abort();
	*p = n++;
	return p;
}

int main(void) {
	struct hashset *s = hashset_create(NULL, hash_int,cmp_int);
	int a[3] = {1200,2400,3600};
	size_t i;
	for (i = 0; i < 5000; i++) {
		assert(hashset_add(s, next_int()));
	}
	for (i = 0; i < 3; i++) {
		assert(hashset_contains(s, &a[i]));
		hashset_remove(s, &a[i]);
	}
	assert(hashset_count(s) == 4997);
	return 0;
}
