/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef TRANSSET_H
#define TRANSET_H

struct trans_set;
struct trans;

struct trans_iter {
	struct set_iter iter;
};

struct trans_set *
trans_set_create(int (*cmp)(const void *a, const void *b));

void
trans_set_free(struct trans_set *set);

struct trans *
trans_set_add(struct trans_set *set, struct trans *item);

struct trans *
trans_set_contains(const struct trans_set *set, const struct trans *item);

void
trans_set_clear(struct trans_set *set);

struct trans *
trans_set_first(const struct trans_set *set, struct trans_iter *it);

struct trans *
trans_set_next(struct trans_iter *it);

#endif

