/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef INTERNEDSTATESET_H
#define INTERNEDSTATESET_H

struct fsm_alloc;
struct state_set;

/* This is a wrapper for state_set instances, which creates and caches
 * sets created by adding a single state to existing sets. As long as
 * the pool is active, the state sets themselves should not be
 * modified. */

/* Opaque types */
struct interned_state_set;
struct interned_state_set_pool;

/* Allocate an interned state set pool. */
struct interned_state_set_pool *
interned_state_set_pool_alloc(const struct fsm_alloc *a);

/* Free the pool.
 * Any state_sets that have not been had a referenced outside
 * the pool marked live via `interned_state_set_retain` will
 * be freed with it. */
void
interned_state_set_pool_free(struct interned_state_set_pool *pool);

/* Get a handle to the empty state set.
 * Returns NULL on alloc failure. */
struct interned_state_set *
interned_state_set_empty(struct interned_state_set_pool *pool);

/* Get a handle to the state set representing set with id added.
 * Returns an opaque set instance, or NULL on alloc failure.
 *
 * If an existing state set is recreated, the existing one will
 * be reused. */
struct interned_state_set *
interned_state_set_add(struct interned_state_set_pool *pool,
    struct interned_state_set *set, fsm_state_t state);

/* Get a reference to the state_set managed by the pool.
 * Note that it should not be modified while the pool is active.
 *
 * Calling this also informs the pool that there is a live reference
 * to the state_set outside the pool, so freeing the pool will no
 * longer free the state_set. */
struct state_set *
interned_state_set_retain(struct interned_state_set *set);

/* Inform the pool that the reference to the state_set outside
 * the pool is no longer being used. If there are no longer any
 * live references, the state_set will be freed with the pool.
 *
 * This is mostly useful if debugging code grabs a reference to
 * the state_set to print it or something, then lets go of it. */
void
interned_state_set_release(struct interned_state_set *set);

#endif
