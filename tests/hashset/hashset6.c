/*
 * Copyright 2018 Shannon Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <adt/hashset.h>

typedef int item_t;

#include "hashset.inc"

static int
cmp_int(const void *a, const void *b)
{
	const int *pa = * (const int * const *) a;
	const int *pb = * (const int * const *) b;

	if (*pa > *pb)      return +1;
	else if (*pa < *pb) return -1;
	else                return  0;
}

static unsigned long
hash_int(const void *a)
{
	return hashrec(a, sizeof * (const int *) a);
}

int *next_int(void) {
	static int n = 0;
	int *p = malloc(sizeof *p);
	if (p == NULL) abort();
	*p = n++;
	return p;
}

enum { COUNT = 5000U };

int main(void) {
	struct hashset *s = hashset_create(NULL, hash_int, cmp_int);
	struct hashset_iter iter;
	size_t i;
	int *p;

	assert(s != NULL);

	for (i = 0; i < COUNT; i++) {
		int *itm = next_int();
		assert(itm != NULL);

		assert(hashset_add(s, itm));
	}

	for (i = 0, p = hashset_first(s, &iter); i < COUNT; i++, hashset_next(&iter)) {
		assert(p);
		if (i < COUNT-1) {
			assert(hashset_hasnext(&iter));
		} else {
			assert(!hashset_hasnext(&iter));
		}
	}

	for (p = hashset_first(s, &iter); p != NULL; p = hashset_next(&iter)) {
		free(p);
	}

	hashset_free(s);
	return 0;
}
