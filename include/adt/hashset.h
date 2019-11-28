/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ADT_HASHSET_H
#define ADT_HASHSET_H

struct hashset_iter {
	size_t i;
	const struct hashset *hashset;
};

unsigned long
hashstates(const fsm_state_t *states, size_t n);

#endif

