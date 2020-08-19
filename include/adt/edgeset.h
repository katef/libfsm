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
	size_t i;
	const struct edge_set *set;
};

struct edge_ordered_iter {
	const struct edge_set *set;
	size_t pos;
	size_t steps;
	unsigned char symbol;
	uint64_t symbols_used[4];
};

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
edge_set_add_state_set(struct edge_set **setp, const struct fsm_alloc *alloc,
	unsigned char symbol, const struct state_set *state_set);

int
edge_set_find(const struct edge_set *set, unsigned char symbol,
	struct fsm_edge *e);

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
    fsm_state_remap_fun *remap, const void *opaque);

void
edge_set_reset(const struct edge_set *set, struct edge_iter *it);

int
edge_set_next(struct edge_iter *it, struct fsm_edge *e);

void
edge_set_rebase(struct edge_set **setp, fsm_state_t base);

void
edge_set_replace_state(struct edge_set **setp, fsm_state_t old, fsm_state_t new);

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

