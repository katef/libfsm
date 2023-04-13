/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ADT_EDGESET_H
#define ADT_EDGESET_H

#include <stdint.h>

struct bm;
struct set;
struct fsm_alloc;
struct fsm_edge;
struct edge_set;
struct state_set;

struct edge_iter {
	size_t i, j;
	const struct edge_set *set;
};

struct edge_ordered_iter {
	const struct edge_set *set;
	size_t pos;
	size_t steps;
	unsigned char symbol;
	uint64_t symbols_used[4];
};

enum edge_group_iter_type {
	EDGE_GROUP_ITER_ALL,
	EDGE_GROUP_ITER_UNIQUE
};

struct edge_group_iter {
	const struct edge_set *set;
	unsigned flag;
	size_t i;
	uint64_t internal[256/64];
};

/* TODO: symbols: macros for bit flags, first, count, etc. */

struct edge_group_iter_info {
	int unique;
	fsm_state_t to;
	uint64_t symbols[256/64];
};

/* Reset an iterator for groups of edges.
 * If iter_type is set to EDGE_GROUP_ITER_UNIQUE,
 * edges with labels that only appear once will be
 * yielded with unique set to 1, all others set to 0.
 * If iter_type is EDGE_GROUP_ITER_ALL, they will not
 * be separated, and unique will not be set. */
void
edge_set_group_iter_reset(const struct edge_set *s,
    enum edge_group_iter_type iter_type,
    struct edge_group_iter *egi);

int
edge_set_group_iter_next(struct edge_group_iter *egi,
    struct edge_group_iter_info *eg);

/* Opaque struct type for edge iterator,
 * which does extra processing upfront to iterate over
 * edges in lexicographically ascending order; the
 * edge_iter iterator is unordered. */
struct edge_iter_ordered;

/* Create an empty edge set.
 * This currently returns a NULL pointer. */
struct edge_set *
edge_set_new(void);

void
edge_set_free(const struct fsm_alloc *a, struct edge_set *set);

int
edge_set_add(struct edge_set **set, const struct fsm_alloc *alloc,
	unsigned char symbol, fsm_state_t state);

int
edge_set_add_bulk(struct edge_set **pset, const struct fsm_alloc *alloc,
	uint64_t symbols[256/64], fsm_state_t state);

/* Notify the data structure that it wis likely to soon need to grow
 * to fit N more bulk edge groups. This can avoid resizing multiple times
 * in smaller increments. */
int
edge_set_advise_growth(struct edge_set **pset, const struct fsm_alloc *alloc,
    size_t count);

int
edge_set_add_state_set(struct edge_set **setp, const struct fsm_alloc *alloc,
	unsigned char symbol, const struct state_set *state_set);

int
edge_set_find(const struct edge_set *set, unsigned char symbol,
	struct fsm_edge *e);

/* Check whether an edge_set has an edge with a particular label. Return
 * 0 for not found, otherwise set *to_state, and note any other labels
 * in labels[] that lead to states that are in the same EC group according
 * to state_ecs[], then return 1.
 *
 * This interface exists specifically so that fsm_minimise can use
 * bit parallelelism to speed up some of its partitioning, it should
 * not be used by anything else. */
int
edge_set_check_edges_with_EC_mapping(const struct edge_set *set,
    unsigned char label, size_t ec_map_count, fsm_state_t *state_ecs,
    fsm_state_t *to_state, uint64_t labels[256/64]);

int
edge_set_contains(const struct edge_set *set, unsigned char symbol);

int
edge_set_hasnondeterminism(const struct edge_set *set, struct bm *bm);

int
edge_set_transition(const struct edge_set *set, unsigned char symbol,
	fsm_state_t *state);

size_t
edge_set_count(const struct edge_set *set);

/* Note: this should actually union src into *dst, if *dst already
 * exists, not replace it. */
int
edge_set_copy(struct edge_set **dst, const struct fsm_alloc *alloc,
	const struct edge_set *src);

void
edge_set_remove(struct edge_set **set, unsigned char symbol);

void
edge_set_remove_state(struct edge_set **set, fsm_state_t state);

int
edge_set_compact(struct edge_set **set, const struct fsm_alloc *alloc,
    fsm_state_remap_fun *remap, const void *opaque);

void
edge_set_reset(const struct edge_set *set, struct edge_iter *it);

int
edge_set_next(struct edge_iter *it, struct fsm_edge *e);

void
edge_set_rebase(struct edge_set **setp, fsm_state_t base);

int
edge_set_replace_state(struct edge_set **setp,
    const struct fsm_alloc *alloc, fsm_state_t old, fsm_state_t new);

int
edge_set_empty(const struct edge_set *s);


/* Initialize an ordered edge_set iterator, positioning it at the first
 * edge with a particular symbol. edge_set_ordered_iter_next will get
 * successive edges with that symbol, then lexicographically following
 * symbols (if any). */
void
edge_set_ordered_iter_reset_to(const struct edge_set *set,
    struct edge_ordered_iter *eoi, unsigned char symbol);

/* Reset an ordered iterator, equivalent to
 * edge_set_ordered_iter_reset_to(set, eoi, '\0'). */
void
edge_set_ordered_iter_reset(const struct edge_set *set,
    struct edge_ordered_iter *eoi);

/* Get the next edge from an ordered iterator and return 1,
 * or return 0 when no more are available. */
int
edge_set_ordered_iter_next(struct edge_ordered_iter *eoi, struct fsm_edge *e);

#endif

