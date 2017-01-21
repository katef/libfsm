/* $Id$ */

#ifndef SET_H
#define SET_H

struct set;

struct set_iter {
	size_t cursor;
	struct set *set;
};

void *
set_addelem(struct set **set, void *elem);

void
set_remove(struct set **set, void *elem);

void
set_free(struct set *set);

/*
 * Find if a elem is in a set.
 */
int
set_contains(const struct set *set, const void *elem);

/*
 * Find if a is a subset of b
 */
int
subsetof(const struct set *a, const struct set *b);

/*
 * Compare two sets of elems for equality.
 */
int
set_equal(const struct set *a, const struct set *b);

void
set_merge(struct set **dst, struct set *src);

int
set_empty(struct set *set);

void *
set_first(struct set *set, struct set_iter *i);

void *
set_next(struct set_iter *i);

int
set_hasnext(struct set_iter *i);

#endif

