/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef INTERNEDSTATESET_H
#define INTERNEDSTATESET_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fsm/fsm.h>

struct fsm_alloc;
struct state_set;

/* This is a wrapper for state_set instances, which creates and caches
 * sets created by adding a single state to existing sets. As long as
 * the pool is active, the state sets themselves should not be
 * modified. */

/* Opaque types */
struct interned_state_set_pool;

/* Numeric ID for an internal node. This is only meaningful
 * when used with the same interned_state_set_pool. */
typedef uint64_t interned_state_set_id;

/* Allocate an interned state set pool. */
struct interned_state_set_pool *
interned_state_set_pool_alloc(const struct fsm_alloc *a);

/* Free the pool.
 * Any state_sets on the interned state sets within the pool
 * will also be freed. */
void
interned_state_set_pool_free(struct interned_state_set_pool *pool);

/* Save a state set and get a unique identifier that can refer
 * to it later. If there is more than one instance of the same
 * set of states, reuse the existing identifier.
 *
 * Note: states[] must be sorted and must not contain duplicates. */
bool
interned_state_set_intern_set(struct interned_state_set_pool *pool,
	size_t state_count, const fsm_state_t *states,
	interned_state_set_id *result);

/* Get the state_set associated with an interned ID.
 * If the state_set has already been built, return the saved instance. */
struct state_set *
interned_state_set_get_state_set(struct interned_state_set_pool *pool,
    interned_state_set_id iss_id);

/* Dump the list of states associated with a set ID. For debugging. */
void
interned_state_set_dump(FILE *f, const struct interned_state_set_pool *pool,
    interned_state_set_id set_id);

#endif
