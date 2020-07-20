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
struct state_set;

struct edge_iter {
	size_t i;
	const struct edge_set *set;
};

void
edge_set_free(struct edge_set *set);

int
edge_set_add(struct edge_set **set, const struct fsm_alloc *alloc,
	unsigned char symbol, fsm_state_t state);

int
edge_set_add_state_set(struct edge_set **setp, const struct fsm_alloc *alloc,
	unsigned char symbol, const struct state_set *state_set);

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
edge_set_copy(struct edge_set **dst, const struct fsm_alloc *alloc,
	const struct edge_set *src);

void
edge_set_remove(struct edge_set **set, unsigned char symbol);

void
edge_set_remove_state(struct edge_set **set, fsm_state_t state);

void
edge_set_compact(struct edge_set **set,
    fsm_state_remap_fun *remap, void *opaque);

void
edge_set_reset(struct edge_set *set, struct edge_iter *it);

int
edge_set_next(struct edge_iter *it, struct fsm_edge *e);

void
edge_set_rebase(struct edge_set **setp, fsm_state_t base);

void
edge_set_replace_state(struct edge_set **setp, fsm_state_t old, fsm_state_t new);

int
edge_set_empty(const struct edge_set *s);

#endif

