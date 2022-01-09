/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef INTERNEDSTATESET_H
#define INTERNEDSTATESET_H

#include <stdint.h>

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
typedef uint32_t interned_state_set_id;

/* Allocate an interned state set pool. */
struct interned_state_set_pool *
interned_state_set_pool_alloc(const struct fsm_alloc *a);

/* Free the pool.
 * Any state_sets that have not been had a referenced outside
 * the pool marked live via `interned_state_set_retain` will
 * be freed with it. */
void
interned_state_set_pool_free(struct interned_state_set_pool *pool);

/* Get a handle to the empty state set. Can not fail. */
interned_state_set_id
interned_state_set_empty(struct interned_state_set_pool *pool);

/* Get a handle to the state set representing set with id added.
 * Returns 1 and sets *result to an opaque set instance, or
 * returns 0 on alloc failure.
 *
 * This takes a pointer to set_id because it transfers ownership
 * of the reference. An opaque `NO_ID` value will be written into
 * the pointer passed in.
 *
 * If an existing state set is recreated, the existing one will
 * be reused. */
int
interned_state_set_add(struct interned_state_set_pool *pool,
    interned_state_set_id *set_id, fsm_state_t state,
    interned_state_set_id *result);

/* Get a reference to the state_set managed by the pool.
 * Note that it should not be modified while the pool is active.
 *
 * Calling this also informs the pool that there is a live reference
 * to the state_set outside the pool, so freeing the pool will no
 * longer free the state_set. */
struct state_set *
interned_state_set_retain(struct interned_state_set_pool *pool,
    interned_state_set_id set_id);

/* Inform the pool that the reference to the state_set outside
 * the pool is no longer being used. If there are no longer any
 * live references, the state_set will be freed with the pool.
 * An opaque `NO_ID` value will be written into the set_id pointer.
 *
 * This is mostly useful if debugging code grabs a reference to
 * the state_set to print it or something, then lets go of it. */
void
interned_state_set_release(struct interned_state_set_pool *pool,
    interned_state_set_id *set_id);

void
interned_state_set_dump(struct interned_state_set_pool *pool,
    interned_state_set_id set_id);

#endif
