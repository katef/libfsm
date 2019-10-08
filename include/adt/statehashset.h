/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ADT_STATEHASHSET_H
#define ADT_STATEHASHSET_H

struct fsm_alloc;
struct state_hashset;
struct fsm_state;

struct state_hashset_iter {
	struct hashset_iter iter;
};

struct state_hashset *
state_hashset_create(const struct fsm_alloc *a,
	unsigned long (*hash)(const void *a),
	int (*cmp)(const void *a, const void *b));

void
state_hashset_free(struct state_hashset *hashset);

struct fsm_state *
state_hashset_add(struct state_hashset *hashset, struct fsm_state *item);

struct fsm_state *
state_hashset_contains(const struct state_hashset *hashset, const struct fsm_state *item);

void
state_hashset_clear(struct state_hashset *hashset);

struct fsm_state *
state_hashset_first(const struct state_hashset *hashset, struct state_hashset_iter *it);

struct fsm_state *
state_hashset_next(struct state_hashset_iter *it);

#endif

