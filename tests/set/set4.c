/*
 * Copyright 2017 Ed Kellett
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

int *next_int(void) {
	static int n = 0;
	int *p = malloc(sizeof *p);
	if (p == NULL) abort();
	*p = n++;
	return p;
}

enum { COUNT = 5000U };

int main(void) {
	struct set *s = set_create(NULL, cmp_int);
	size_t i;
	int **plist;

	assert(s != NULL);

	plist = calloc(COUNT, sizeof *plist);
	assert(plist != NULL);

	for (i = 0; i < COUNT; i++) {
		int *itm = next_int();
		plist[i] = itm;
		assert(set_add(s, itm));
	}
	assert(set_count(s) == COUNT);

	for (i=0; i < COUNT; i++) {
		free(plist[i]);
	}
	free(plist);

	set_free(s);
	return 0;
}
