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

int
hashset_contains(const struct hashset *set, const void *item)
{
	unsigned long h = hash_int(item);
	size_t b = 0;

	assert(set != NULL);

	return finditem(set, h, item, &b);
}

int *next_int(int reset) {
	static int n = 0;
	int *p;

	if (reset) {
		n = 0;
		return NULL;
	}

	p = malloc(sizeof *p);
	if (p == NULL) abort();
	*p = n++;
	return p;
}

int main(void) {
	struct hashset *s = hashset_create(NULL, hash_int, cmp_int);
	size_t i;
	int **plist;

	plist = malloc(10000 * sizeof *plist);
	assert(plist != NULL);

	for (i = 0; i < 5000; i++) {
		int *itm = next_int(0);
		plist[i] = itm;
		assert(hashset_add(s, itm));
	}
	assert(hashset_count(s) == 5000);

	next_int(1);
	for (i = 0; i < 5000; i++) {
		int *itm = next_int(0);
		plist[5000+i] = itm;
		assert(hashset_add(s, itm));
	}
	assert(hashset_count(s) == 5000);

	for (i=0; i < 10000; i++) {
		free(plist[i]);
	}
	free(plist);

	hashset_free(s);
	return 0;
}
