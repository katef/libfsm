/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>

#include <adt/alloc.h>
#include <adt/stateset.h>
#include <adt/internedstateset.h>

#include "common/libfsm_common.h"

#define ISS_BUCKET_DEF_CEIL 4
#define CACHE_BUCKET_DEF_CEIL 32
#define LOG_INTERNEDSTATESET 0

#if LOG_INTERNEDSTATESET
#include <stdio.h>
#endif

/* 32-bit approximation of golden ratio, used for hashing */
#define PHI32 0x9e3779b9

#define NO_ISS ((struct interned_state_set *)-1)
#define BUCKET_STATE_NONE ((fsm_state_t)-1)

struct interned_state_set {
	unsigned refcount;
	unsigned long hash;
	struct state_set *set;

	/* Cache of other interned state sets derived directly from the
	 * current set by adding one state. These are weak references
	 * and aren't reflected in the refcount. */
	struct iss_kids {
		unsigned bucket_count;
		unsigned buckets_used;
		struct kid_bucket {
			fsm_state_t state; /* or BUCKET_STATE_NONE */
			struct interned_state_set *kid;
		} *buckets;
	} kids;
};

struct interned_state_set_pool {
	const struct fsm_alloc *a;

	struct interned_state_set *empty;

	/* Cache of state sets. */
	struct issp_cache {
		unsigned bucket_count;
		unsigned buckets_used;
		struct cache_bucket {
			unsigned long hash;
			struct interned_state_set *iss; /* NO_ISS if empty */
		} *buckets;
	} cache;
};

static int
grow_cache_buckets(struct interned_state_set_pool *pool);

static int
grow_kid_cache(const struct fsm_alloc *a, struct interned_state_set *set);

static void
add_kid_to_iss_cache(struct interned_state_set_pool *pool,
    struct interned_state_set *set, fsm_state_t state,
    struct interned_state_set *kid);

struct interned_state_set_pool *
interned_state_set_pool_alloc(const struct fsm_alloc *a)
{
	struct interned_state_set_pool *res;
	struct cache_bucket *buckets = NULL;
	struct interned_state_set *empty_i = NULL;
	struct state_set *empty_ss;

	size_t i;

	res = f_calloc(a, 1, sizeof(*res));
	if (res == NULL) {
		goto cleanup;
	}

	buckets = f_malloc(a,
	    CACHE_BUCKET_DEF_CEIL * sizeof(buckets[0]));
	if (buckets == NULL) {
		goto cleanup;
	}
	for (i = 0; i < CACHE_BUCKET_DEF_CEIL; i++) {
		buckets[i].iss = NO_ISS;
	}

	empty_i = f_calloc(a, 1, sizeof(*empty_i));
	if (empty_i == NULL) {
		goto cleanup;
	}

	/* Currently, a NULL pointer is treated as a boxed
	 * empty state set. There isn't a constructor. */
	empty_ss = NULL;	/* can't fail */

	empty_i->refcount = 1;
	empty_i->hash = state_set_hash(empty_ss);
	empty_i->set = empty_ss;
	assert(empty_i->kids.bucket_count == 0); /* starts unallocated */

	res->a = a;
	res->cache.bucket_count = CACHE_BUCKET_DEF_CEIL;
	res->cache.buckets = buckets;
	{
		const unsigned long mask = CACHE_BUCKET_DEF_CEIL - 1;
		struct cache_bucket *b = &res->cache.buckets[empty_i->hash & mask];
		assert(b->iss == NO_ISS);
		b->hash = empty_i->hash;
		b->iss = empty_i;
	}
	res->empty = empty_i;

	return res;

cleanup:
	if (res != NULL) {
		f_free(a, res);
	}
	if (buckets != NULL) {
		f_free(a, buckets);
	}
	if (empty_i != NULL) {
		f_free(a, empty_i);
	}
	return NULL;
}

void
interned_state_set_pool_free(struct interned_state_set_pool *pool)
{
	size_t i;
	if (pool == NULL) {
		return;
	}

	for (i = 0; i < pool->cache.bucket_count; i++) {
		struct cache_bucket *b = &pool->cache.buckets[i];
		if (b->iss != NO_ISS) {
			struct interned_state_set *iss = b->iss;
			iss->refcount--;
			if (iss->refcount == 0) {
				state_set_free(iss->set);
				f_free(pool->a, iss->kids.buckets);
				f_free(pool->a, iss);
			}
		}
	}

	f_free(pool->a, pool->cache.buckets);
	f_free(pool->a, pool);
}

struct interned_state_set *
interned_state_set_empty(struct interned_state_set_pool *pool)
{
	return pool->empty;
}

SUPPRESS_EXPECTED_UNSIGNED_INTEGER_OVERFLOW()
static unsigned long
hash_state(fsm_state_t state)
{
	return PHI32 * (state + 1);
}

struct interned_state_set *
interned_state_set_add(struct interned_state_set_pool *pool,
    struct interned_state_set *set, fsm_state_t state)
{
	struct interned_state_set *res;
	size_t i;
	unsigned long h, mask;
	struct state_set *ss = NULL;

#if LOG_INTERNEDSTATESET
	fprintf(stderr, "interned_state_set_add: adding state %d to %p\n", state, (void *)set);
#endif

	assert(pool != NULL);
	assert(set != NULL);
	assert(state != BUCKET_STATE_NONE);

	/* First, check if the interned state set has a known
	 * child derived by adding that id. */
	if (set->kids.bucket_count > 0) {
		struct iss_kids *kcache = &set->kids;
		mask = kcache->bucket_count - 1;
		h = hash_state(state);
		for (i = 0; i < kcache->bucket_count; i++) {
			struct kid_bucket *kb = &kcache->buckets[(h + i) & mask];
			if (kb->state == BUCKET_STATE_NONE) {
				break; /* not found */
			} else if (kb->state == state) {
#if LOG_INTERNEDSTATESET
				fprintf(stderr, "interned_state_set_add: KID_CACHE_HIT: %p on %d\n",
				    (void *)kb->kid, state);
#endif
				return kb->kid;
			} else {
				/* collision, continue */
			}
		}
	}

	/* If not, check the global cache, and add the child
	 * if present.
	 *
	 * Because currently hashing isn't incremental and depends on
	 * constructing the set, we need to make a copy just to check if
	 * it already exists. This is expensive. */
	if (!state_set_copy(&ss, pool->a, set->set)) {
		return NULL;
	}
	if (!state_set_add(&ss, pool->a, state)) {
		return NULL;
	}

	h = state_set_hash(ss);
	mask = pool->cache.bucket_count - 1;
	for (i = 0; i < pool->cache.bucket_count; i++) {
		struct cache_bucket *cb = &pool->cache.buckets[(h + i) & mask];
		if (cb->iss == NO_ISS) {
			break;	/* not found */
		} else if (cb->hash == h && 0 == state_set_cmp(ss, cb->iss->set)) {
			state_set_free(ss);
#if LOG_INTERNEDSTATESET
			fprintf(stderr, "interned_state_set_add: GLOBAL_CACHE HIT: %p\n",
			    (void *)cb->iss);
#endif

			add_kid_to_iss_cache(pool, set, state, cb->iss);
			return cb->iss;
		} else {
			/* collision, continue */
		}
	}

	/* If not, create one and add it to both caches. */
	res = f_calloc(pool->a, 1, sizeof(*res));
	if (res == NULL) {
		return NULL;
	}
	res->refcount = 1;
	res->hash = h;
	res->set = ss;

#if LOG_INTERNEDSTATESET > 1
	fprintf(stderr, "interned_state_set_add: cache is using %d / %d\n",
	    pool->cache.buckets_used, pool->cache.bucket_count);
#endif
	if (pool->cache.buckets_used == pool->cache.bucket_count/2) {
		if (!grow_cache_buckets(pool)) {
			return NULL;
		}
	}

	for (i = 0; i < pool->cache.bucket_count; i++) {
		struct cache_bucket *cb = &pool->cache.buckets[(h + i) & mask];
		if (cb->iss == NO_ISS) {
			pool->cache.buckets_used++;
			cb->hash = h;
			cb->iss = res;

			add_kid_to_iss_cache(pool, set, state, res);
			return res;
		} else {
			/* collision, continue */
		}
	}
	assert(i < pool->cache.bucket_count); /* found a spot */

	add_kid_to_iss_cache(pool, set, state, res);
	return res;
}

static int
grow_cache_buckets(struct interned_state_set_pool *pool)
{
	size_t o_i, n_i;
	const size_t nceil = 2 * pool->cache.bucket_count;
	const size_t nmask = nceil - 1;
	struct cache_bucket *nbuckets;
	assert(nceil > 0);
#if LOG_INTERNEDSTATESET
	fprintf(stderr, "grow_cache_buckets: %u -> %lu\n",
	    pool->cache.bucket_count, nceil);
#endif

	nbuckets = f_malloc(pool->a, nceil * sizeof(nbuckets[0]));
	if (nbuckets == NULL) {
		return 0;
	}

	for (n_i = 0; n_i < nceil; n_i++) {
		nbuckets[n_i].iss = NO_ISS;
	}

	for (o_i = 0; o_i < pool->cache.bucket_count; o_i++) {
		struct cache_bucket *cb = &pool->cache.buckets[o_i];
		if (cb->iss == NO_ISS) {
			continue;
		}

		for (n_i = 0; n_i < nceil; n_i++) {
			struct cache_bucket *ncb = &nbuckets[(cb->hash + n_i) & nmask];
			if (ncb->iss == NO_ISS) {
				ncb->iss = cb->iss;
				ncb->hash = cb->hash;
				break;
			} else {
				/* collision, skip */
			}
		}
		assert(n_i < nceil); /* found a new position */
	}

	f_free(pool->a, pool->cache.buckets);
	pool->cache.buckets = nbuckets;
	pool->cache.bucket_count = nceil;
	return 1;
}

/* This failing will degrade performance but not correctness,
 * and will try to add it again next time. */
static void
add_kid_to_iss_cache(struct interned_state_set_pool *pool,
    struct interned_state_set *set, fsm_state_t state,
    struct interned_state_set *kid)
{
	size_t i;
	const unsigned long h = hash_state(state);
	unsigned long mask;

#if LOG_INTERNEDSTATESET
	fprintf(stderr, "add_kid_to_iss_cache: set %p, state %d, kid %p\n",
	    (void *)set, state, (void *)kid);
#endif

	if (set->kids.bucket_count == 0) {
		assert(set->kids.buckets == NULL);
		assert(set->kids.buckets_used == 0);
		set->kids.buckets = f_malloc(pool->a,
		    ISS_BUCKET_DEF_CEIL * sizeof(set->kids.buckets[0]));
		if (set->kids.buckets == NULL) {
			return;
		}
		set->kids.bucket_count = ISS_BUCKET_DEF_CEIL;
		for (i = 0; i < ISS_BUCKET_DEF_CEIL; i++) {
			set->kids.buckets[i].state = BUCKET_STATE_NONE;
		}
	}

	assert(set->kids.bucket_count > 0);
	if (set->kids.buckets_used == set->kids.bucket_count/2) {
		if (!grow_kid_cache(pool->a, set)) {
			return;
		}
	}

	mask = set->kids.bucket_count - 1;
	for (i = 0; i < set->kids.bucket_count; i++) {
		struct kid_bucket *kb = &set->kids.buckets[(h + i) & mask];
		if (kb->state == BUCKET_STATE_NONE) {
			kb->state = state;
			kb->kid = kid;
			set->kids.buckets_used++;
			break;
		} else if (kb->state == state) {
			assert(!"already present");
		} else {
			/* collision, skip */
		}
	}
}

static int
grow_kid_cache(const struct fsm_alloc *a, struct interned_state_set *set)
{
	const size_t nceil = 2 * set->kids.bucket_count;
	struct kid_bucket *nbuckets;
	size_t o_i, n_i;
	const size_t nmask = nceil - 1;
	assert(nceil > 0);

	nbuckets = f_malloc(a, nceil * sizeof(nbuckets[0]));
	if (nbuckets == NULL) {
		return 0;
	}

	for (n_i = 0; n_i < nceil; n_i++) {
		nbuckets[n_i].state = BUCKET_STATE_NONE;
	}

	for (o_i = 0; o_i < set->kids.bucket_count; o_i++) {
		struct kid_bucket *ob = &set->kids.buckets[o_i];
		unsigned long h;
		if (ob->state == BUCKET_STATE_NONE) {
			continue;
		}

		h = hash_state(ob->state);
		for (n_i = 0; n_i < nceil; n_i++) {
			struct kid_bucket *nb = &nbuckets[(h + n_i) & nmask];
			if (nb->state == BUCKET_STATE_NONE) {
				nb->state = ob->state;
				nb->kid = ob->kid;
				break;
			}
		}
		assert(n_i < nceil); /* found a spot */
	}

	f_free(a, set->kids.buckets);
	set->kids.bucket_count = nceil;
	set->kids.buckets = nbuckets;
	return 1;
}

struct state_set *
interned_state_set_retain(struct interned_state_set *set)
{
	struct state_set *res;
	assert(set != NULL);
	assert(set->refcount >= 1); /* pool has a reference */
	set->refcount++;
	res = set->set;
	return res;
}

void
interned_state_set_release(struct interned_state_set *set)
{
	assert(set != NULL);
	assert(set->refcount > 1); /* pool still has a reference */
	set->refcount--;
}
