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
	/* ensure that a has enough elements that the table has to be
	 * rehashed a few times
	 */
	int a[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

	assert(s != NULL);

	/* add 'em in */
	assert(hashset_add(s, &a[0]));
	assert(hashset_add(s, &a[1]));
	assert(hashset_add(s, &a[2]));
	assert(hashset_add(s, &a[3]));

	assert(hashset_add(s, &a[4]));
	assert(hashset_add(s, &a[5]));
	assert(hashset_add(s, &a[6]));
	assert(hashset_add(s, &a[7]));

	assert(hashset_add(s, &a[8]));
	assert(hashset_add(s, &a[9]));
	assert(hashset_add(s, &a[10]));
	assert(hashset_add(s, &a[11]));

	assert(hashset_add(s, &a[12]));
	assert(hashset_add(s, &a[13]));
	assert(hashset_add(s, &a[14]));
	assert(hashset_add(s, &a[15]));

	/* check that they're in */
	assert(hashset_contains(s, &a[0]));
	assert(hashset_contains(s, &a[1]));
	assert(hashset_contains(s, &a[2]));
	assert(hashset_contains(s, &a[3]));

	assert(hashset_contains(s, &a[4]));
	assert(hashset_contains(s, &a[5]));
	assert(hashset_contains(s, &a[6]));
	assert(hashset_contains(s, &a[7]));

	assert(hashset_contains(s, &a[8]));
	assert(hashset_contains(s, &a[9]));
	assert(hashset_contains(s, &a[10]));
	assert(hashset_contains(s, &a[11]));

	assert(hashset_contains(s, &a[12]));
	assert(hashset_contains(s, &a[13]));
	assert(hashset_contains(s, &a[14]));
	assert(hashset_contains(s, &a[15]));

	hashset_free(s);
	return 0;
}
