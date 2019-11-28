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

int main(void) {
	struct hashset *s = hashset_create(NULL, hash_state, cmp_states);
	fsm_state_t a[3] = {1, 2, 3};
	assert(hashset_add(s, &a[0]));
	assert(hashset_add(s, &a[1]));
	assert(hashset_add(s, &a[2]));
	return 0;
}
