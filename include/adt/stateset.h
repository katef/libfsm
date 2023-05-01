/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ADT_STATESET_H
#define ADT_STATESET_H

#include <stdint.h>

struct set;
struct fsm_alloc;
struct state_set;

struct state_iter {
	size_t i;
	const struct state_set *set;
};

int
state_set_has(const struct fsm *fsm, const struct state_set *set,
	int (*predicate)(const struct fsm *, fsm_state_t));

void
state_set_free(struct state_set *set);

int
state_set_add(struct state_set **set, const struct fsm_alloc *alloc,
	fsm_state_t state);

int
state_set_add_bulk(struct state_set **set, const struct fsm_alloc *alloc,
	const fsm_state_t *a, size_t n);

int
state_set_copy(struct state_set **dst, const struct fsm_alloc *alloc,
	const struct state_set *src);

void
state_set_compact(struct state_set **set,
    fsm_state_remap_fun *remap, void *opaque);

void
state_set_remove(struct state_set **set, fsm_state_t state);

int
state_set_empty(const struct state_set *s);

fsm_state_t
state_set_only(const struct state_set *s);

int
state_set_contains(const struct state_set *set, fsm_state_t state);

size_t
state_set_count(const struct state_set *set);

void
state_set_reset(const struct state_set *set, struct state_iter *it);

int
state_set_next(struct state_iter *it, fsm_state_t *state);

int
state_set_cmp(const struct state_set *a, const struct state_set *b);

const fsm_state_t *
state_set_array(const struct state_set *set);

void
state_set_rebase(struct state_set **set, fsm_state_t base);

void
state_set_replace(struct state_set **set, fsm_state_t old, fsm_state_t new);

uint64_t
state_set_hash(const struct state_set *set);

#endif

