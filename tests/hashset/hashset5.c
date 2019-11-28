/*
 * Copyright 2018 Shannon Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include <adt/hashset.h>

typedef fsm_state_t item_t;

#include "hashset.inc"

static int
cmp_states(const void *a, const void *b)
{
	const fsm_state_t *pa = * (const fsm_state_t * const *) a;
	const fsm_state_t *pb = * (const fsm_state_t * const *) b;

	if (*pa > *pb)      return +1;
	else if (*pa < *pb) return -1;
	else                return  0;
}

static unsigned long
hash_state(const void *a)
{
	return hashstates(a, 1);
}

int
hashset_contains(const struct hashset *set, const void *item)
{
	unsigned long h = hash_state(item);
	size_t b = 0;

	assert(set != NULL);

	return finditem(set, h, item, &b);
}

fsm_state_t *next_state(void) {
	static fsm_state_t n = 0;
	fsm_state_t *p;

	p = malloc(sizeof *p);
	if (p == NULL) abort();
	*p = n++;
	return p;
}

int main(void) {
	struct hashset *s = hashset_create(NULL, hash_state, cmp_states);
	fsm_state_t a[3] = {1200,2400,3600};
	size_t i;
	for (i = 0; i < 5000; i++) {
		assert(hashset_add(s, next_state()));
	}
	for (i = 0; i < 3; i++) {
		assert(hashset_contains(s, &a[i]));
		hashset_remove(s, &a[i]);
	}
	assert(hashset_count(s) == 4997);
	return 0;
}
