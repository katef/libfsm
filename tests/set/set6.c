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
	struct set_iter iter;
	size_t i;
	int *p;
	for (i = 0; i < 5000; i++) {
		assert(set_add(&s, next_int()));
	}
	for (i = 0, p = set_first(s, &iter); i < 5000; i++, set_next(&iter)) {
		assert(p);
		if (i < 4999) {
			assert(set_hasnext(&iter));
		} else {
			assert(!set_hasnext(&iter));
		}
	}
	return 0;
}
