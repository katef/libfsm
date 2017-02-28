/* $Id$ */

#ifndef SET_H
#define SET_H

struct set;

struct set_iter {
	size_t i;
	const struct set *set;
};

struct set *
set_create(int (*cmp)(const void *a, const void *b));

void *
set_add(struct set **set, void *item);

void
set_remove(struct set **set, void *item);

void
set_free(struct set *set);

void
set_clear(struct set *set);

/*
 * Find if an item is in a set, and return it.
 */
void *
set_contains(const struct set *set, const void *item);

/*
 * Find if a is a subset of b
 */
int
subsetof(const struct set *a, const struct set *b);

/*
 * Compare two sets like memcmp
 */
int
set_cmp(const struct set *a, const struct set *b);

/*
 * Compare two sets for equality.
 */
int
set_equal(const struct set *a, const struct set *b);

void
set_merge(struct set **dst, struct set *src);

int
set_empty(const struct set *set);

void *
set_first(const struct set *set, struct set_iter *it);

void *
set_firstafter(const struct set *set, struct set_iter *it, void *item);

void *
set_next(struct set_iter *it);

/*
 * Return the sole item for a singleton set.
 */
void *
set_only(const struct set *set);

int
set_hasnext(const struct set_iter *it);

#endif

