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

int *next_int(void) {
	static int n = 0;
	int *p = malloc(sizeof *p);
	if (p == NULL) abort();
	*p = n++;
	return p;
}

int main(void) {
	struct set *s = set_create(cmp_int);
	int a[3] = {1200,2400,3600};
	size_t i;
	for (i = 0; i < 5000; i++) {
		assert(set_add(&s, next_int()));
	}
	for (i = 0; i < 3; i++) {
		assert(set_contains(s, &a[i]));
		set_remove(&s, &a[i]);
	}
	assert(set_count(s) == 4997);
	return 0;
}
