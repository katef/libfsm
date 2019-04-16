/*
 * Copyright 2017 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ADT_TUPLESET_H
#define ADT_TUPLESET_H

struct set;
struct fsm_alloc;
struct fsm_state;
struct fsm_walk2_tuple;

struct tuple_iter {
	struct set_iter iter;
};

struct tuple_set *
tuple_set_create(const struct fsm_alloc *a,
	int (*cmp)(const void *, const void *));

void
tuple_set_free(struct tuple_set *set);

struct fsm_walk2_tuple *
tuple_set_contains(const struct tuple_set *set, const struct fsm_walk2_tuple *item);

struct fsm_walk2_tuple *
tuple_set_add(struct tuple_set *set, struct fsm_walk2_tuple *item);

#endif

