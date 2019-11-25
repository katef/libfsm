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

/*
 * Type-independent utility functions
 */

unsigned long
hashrec(const void *p, size_t n);

#endif

