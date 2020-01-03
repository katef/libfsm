/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ADT_EDGESET_H
#define ADT_EDGESET_H

struct bm;
struct set;
struct fsm_alloc;
struct fsm_edge;
struct edge_set;

struct edge_iter {
	size_t i;
	const struct edge_set *set;
};

struct edge_set *
edge_set_create(const struct fsm_alloc *a);

void
edge_set_free(struct edge_set *set);

struct fsm_edge *
edge_set_add(struct edge_set *set, unsigned char symbol,
	fsm_state_t state);

int
edge_set_contains(const struct edge_set *set, unsigned char symbol);

int
edge_set_hasnondeterminism(const struct edge_set *set, struct bm *bm);

int
edge_set_transition(const struct edge_set *set, unsigned char symbol,
	fsm_state_t *state);

size_t
edge_set_count(const struct edge_set *set);

int
edge_set_copy(struct edge_set *dst, const struct edge_set *src);

void
edge_set_remove(struct edge_set *set, unsigned char symbol);

void
edge_set_remove_state(struct edge_set *set, fsm_state_t state);

struct fsm_edge *
edge_set_first(struct edge_set *set, struct edge_iter *it);

struct fsm_edge *
edge_set_next(struct edge_iter *it);

int
edge_set_empty(const struct edge_set *s);

#endif

