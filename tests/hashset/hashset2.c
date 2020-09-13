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

int main(void) {
	struct hashset *s = hashset_create(NULL, hash_int, cmp_int);
	int a = 1;

	assert(s != NULL);

	assert(hashset_add(s, &a));
	assert(hashset_add(s, &a));
	assert(hashset_add(s, &a));

	hashset_remove(s, &a);

	assert(!hashset_contains(s, &a));

	hashset_free(s);
	return 0;
}

