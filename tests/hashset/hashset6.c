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
	struct hashset_iter iter;
	size_t i;
	fsm_state_t *p;
	for (i = 0; i < 5000; i++) {
		assert(hashset_add(s, next_state()));
	}
	for (i = 0, p = hashset_first(s, &iter); i < 5000; i++, hashset_next(&iter)) {
		assert(p);
		if (i < 4999) {
			assert(hashset_hasnext(&iter));
		} else {
			assert(!hashset_hasnext(&iter));
		}
	}
	return 0;
}
