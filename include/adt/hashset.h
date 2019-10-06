/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ADT_HASHSET_H
#define ADT_HASHSET_H

#include <stddef.h>

#define DEFAULT_LOAD 0.66
#define DEFAULT_NBUCKETS 4

struct fsm_alloc;
struct hashset;
struct bucket;

struct hashset_iter {
	size_t i;
	const struct hashset *hashset;
};

struct hashset *
hashset_create(const struct fsm_alloc *a,
	unsigned long (*hash)(const void *a),
	int (*cmp)(const void *a, const void *b));

int
hashset_initialize(const struct fsm_alloc *a,
	struct hashset *hashset, size_t nb, float load,
	unsigned long (*hash)(const void *a),
	int (*cmp)(const void *a, const void *b));

void
hashset_finalize(struct hashset *hashset);

void *
hashset_add(struct hashset *hashset, void *item);

int
hashset_remove(struct hashset *hashset, const void *item);

void
hashset_free(struct hashset *hashset);

size_t
hashset_count(const struct hashset *hashset);

void
hashset_clear(struct hashset *hashset);

/*
 * Find if an item is in a set, and return it.
 */
void *
hashset_contains(const struct hashset *hashset, const void *item);

/*
 * Compare two sets for equality.
 */
int
hashset_equal(const struct hashset *a, const struct hashset *b);

int
hashset_empty(const struct hashset *hashset);

void *
hashset_first(const struct hashset *hashset, struct hashset_iter *it);

void *
hashset_next(struct hashset_iter *it);

/*
 * Return the sole item for a singleton set.
 */
void *
hashset_only(const struct hashset *hashset);

int
hashset_hasnext(struct hashset_iter *it);

/* Common hash functions */

/* hashes pointer value */
unsigned long
hashptr(const void *p);

/* hashes record pointed to by `p` with length n.
 * this includes padding bytes, so treat with care. */
unsigned long
hashrec(const void *p, size_t n);

#endif /* HASHSET_H */

