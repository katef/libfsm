/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ADT_SET_H
#define ADT_SET_H

struct fsm_alloc;
struct set;

struct set_iter {
	size_t i;
	const struct set *set;
};

struct set *
set_create(const struct fsm_alloc *a,
	int (*cmp)(const void *a, const void *b));

struct set *
set_create_singleton(const struct fsm_alloc *a,
	int (*cmp)(const void *a, const void *b), void *item);

struct set *
set_create_from_array(const struct fsm_alloc *a,
	void *items[], size_t n,
	int (*cmp)(const void *a, const void *b),
	int (*bulkcmp)(const void *, const void *));

struct set *
set_copy(const struct set *set);

void *
set_add(struct set *set, void *item);

void
set_remove(struct set *set, const void *item);

void
set_free(struct set *set);

size_t
set_count(const struct set *set);

void
set_clear(struct set *set);

/*
 * Find if an item is in a set, and return it.
 */
void *
set_contains(const struct set *set, const void *item);

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

int
set_empty(const struct set *set);

void *
set_first(const struct set *set, struct set_iter *it);

void *
set_firstafter(const struct set *set, struct set_iter *it, const void *item);

void *
set_next(struct set_iter *it);

/*
 * Return the sole item for a singleton set.
 */
void *
set_only(const struct set *set);

int
set_hasnext(const struct set_iter *it);

const void **
set_array(const struct set *set);

#endif

