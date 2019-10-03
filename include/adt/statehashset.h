/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ADT_STATEHASHSET_H
#define ADT_STATEHASHSET_H

struct fsm_alloc;
struct state_hashset;

struct state_hashset_iter {
	struct hashset_iter iter;
};

struct state_hashset *
state_hashset_create(const struct fsm_alloc *a);

void
state_hashset_free(struct state_hashset *hashset);

int
state_hashset_add(struct state_hashset *hashset, fsm_state_t item);

int
state_hashset_contains(const struct state_hashset *hashset, fsm_state_t item);

#endif

