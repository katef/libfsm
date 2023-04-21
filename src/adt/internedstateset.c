/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include <fsm/fsm.h>

#include <adt/alloc.h>
#include <adt/hash.h>
#include <adt/stateset.h>

#include "common/check.h"

#include <adt/internedstateset.h>

#define LOG_ISS 0

#define NO_ID ((uint32_t)-1)
#define DEF_SETS 8
#define DEF_BUF 64
#define DEF_BUCKETS 256

/* Interner for `struct state_set *`s. */
struct interned_state_set_pool {
	const struct fsm_alloc *alloc;

	/* Buffer for storing lists of state IDs. */
	struct issp_state_set_buf {
		size_t ceil;
		size_t used;
		fsm_state_t *buf;
	} buf;

	/* Unique state sets. */
	struct issp_state_set_descriptors {
		size_t ceil;
		size_t used;
		struct issp_set {
			size_t buf_offset;
			size_t length;
			struct state_set *ss; /* can be NULL */
		} *sets;
	} sets;

	/* Hash table, for quickly finding and reusing
	 * state sets with the same states. */
	struct issp_htab {
		size_t bucket_count; /* power of 2 */
		size_t used;
		uint32_t *buckets; /* or NO_ID */
	} htab;
};

/* It's very common to have state sets with only a single state,
 * particularly when determinising an FSM that is already mostly
 * deterministic. Rather than interning them, just return a specially
 * formatted interned_state_set_id boxing the single state. */
#define SINGLETON_BIT ((uint64_t)1 << 63)
#define IS_SINGLETON(X) (X & SINGLETON_BIT)
#define MASK_SINGLETON(X) (X & ~SINGLETON_BIT)

/* Allocate an interned state set pool. */
struct interned_state_set_pool *
interned_state_set_pool_alloc(const struct fsm_alloc *a)
{
	struct interned_state_set_pool *res = NULL;
	struct issp_set *sets = NULL;
	fsm_state_t *buf = NULL;
	uint32_t *buckets = NULL;

	res = f_calloc(a, 1, sizeof(*res));
	if (res == NULL) { goto cleanup; }

	sets = f_malloc(a, DEF_SETS * sizeof(sets[0]));
	if (sets == NULL) { goto cleanup; }

	buf = f_malloc(a, DEF_BUF * sizeof(buf[0]));
	if (sets == NULL) { goto cleanup; }

	buckets = f_malloc(a, DEF_BUCKETS * sizeof(buckets[0]));
	if (buckets == NULL) { goto cleanup; }

	for (size_t i = 0; i < DEF_BUCKETS; i++) {
		buckets[i] = NO_ID;
	}

	struct interned_state_set_pool p = {
		.alloc = a,
		.sets = {
			.ceil = DEF_SETS,
			.sets = sets,
		},
		.buf = {
			.ceil = DEF_BUF,
			.buf = buf,
		},
		.htab = {
			.bucket_count = DEF_BUCKETS,
			.buckets = buckets,
		},
	};
	memcpy(res, &p, sizeof(p));
	return res;

cleanup:
	f_free(a, sets);
	f_free(a, buf);
	f_free(a, buckets);
	f_free(a, res);
	return NULL;
}

/* Free the pool.
 * Any state_sets on the interned state sets within the pool
 * will also be freed. */
void
interned_state_set_pool_free(struct interned_state_set_pool *pool)
{
	if (pool == NULL) { return; }

#if LOG_ISS
	fprintf(stderr, "%s: sets: %zu/%zu, buf: %zu/%zu state_ids, htab: %zu/%zu buckets used\n",
	    __func__, pool->sets.used, pool->sets.ceil,
	    pool->buf.used, pool->buf.ceil,
	    pool->htab.used, pool->htab.bucket_count);
#endif

	for (size_t i = 0; i < pool->sets.used; i++) {
		if (pool->sets.sets[i].ss != NULL) {
			state_set_free(pool->sets.sets[i].ss);
		}
	}

	f_free(pool->alloc, pool->sets.sets);
	f_free(pool->alloc, pool->buf.buf);
	f_free(pool->alloc, pool->htab.buckets);
	f_free(pool->alloc, pool);
}

static void
dump_tables(FILE *f, const struct interned_state_set_pool *pool)
{
#if LOG_ISS > 1
	fprintf(f, "%s: sets:\n", __func__);
	for (size_t i = 0; i < pool->sets.used; i++) {
		struct issp_set *s = &pool->sets.sets[i];
		fprintf(f, "-- sets[%zu]: length %zd, offset %zd =>",
		    i, s->length, s->buf_offset);
		for (size_t i = 0; i < s->length; i++) {
			fprintf(f, " %u", pool->buf.buf[s->buf_offset + i]);
		}
		fprintf(f, "\n");
	}

	fprintf(f, "%s: buf: %zu/%zu used\n", __func__, pool->buf.used, pool->buf.ceil);

	fprintf(f, "%s: htab: %zu/%zu used\n", __func__, pool->htab.used, pool->htab.bucket_count);
#endif

#if LOG_ISS > 1
	for (size_t i = 0; i < pool->htab.bucket_count; i++) {
		const uint32_t b = pool->htab.buckets[i];
		if (b != NO_ID) {
			fprintf(f, "%s: htab[%zu]: %u\n", __func__, i, b);
		}
	}
#else

	uint8_t count = 0;
	size_t row_count = 0;
	for (size_t i = 0; i < pool->htab.bucket_count; i++) {
		const uint32_t b = pool->htab.buckets[i];
		if (b != NO_ID) {
			count++;
		}
		if ((i & 7) == 7) {
			const char c = (count == 0 ? '.' : '0' + count);
			fprintf(f, "%c", c);
			row_count += count;
			count = 0;
			if ((i & 511) == 511) { /* 64 chars wide */
				fprintf(f, " -- %zu\n", row_count);
				row_count = 0;
			}
		}
	}
	fprintf(f, "\n");
#endif
}

SUPPRESS_EXPECTED_UNSIGNED_INTEGER_OVERFLOW()
static uint64_t
hash_state_ids(size_t count, const fsm_state_t *ids)
{
	uint64_t h = 0;
	for (size_t i = 0; i < count; i++) {
		h = hash_id(ids[i]) + (FSM_PHI_64 * h);
	}
	return h;
}

static bool
grow_htab(struct interned_state_set_pool *pool)
{
	const uint64_t ocount = pool->htab.bucket_count;
	const uint64_t ncount = 2 * ocount;
	const uint64_t nmask = ncount - 1;

	uint32_t *obuckets = pool->htab.buckets;
	uint32_t *nbuckets = f_malloc(pool->alloc,
	    ncount * sizeof(nbuckets[0]));
	if (nbuckets == NULL) { return false; }

	for (size_t i = 0; i < ncount; i++) {
		nbuckets[i] = NO_ID;
	}

	size_t max_collisions = 0;
	for (size_t i = 0; i < ocount; i++) {
		size_t collisions = 0;
		const uint32_t id = obuckets[i];
		if (id == NO_ID) { continue; }

		struct issp_set *s = &pool->sets.sets[id];
		assert(s->buf_offset + s->length <= pool->buf.used);
		const fsm_state_t *buf = &pool->buf.buf[s->buf_offset];

		const uint64_t h = hash_state_ids(s->length, buf);
		for (uint64_t b_i = 0; b_i < ncount; b_i++) {
			uint32_t *nb = &nbuckets[(h + b_i) & nmask];
			if (*nb == NO_ID) {
				*nb = id;
				break;
			} else {
				collisions++;
			}
		}
		if (collisions > max_collisions) {
			max_collisions = collisions;
		}
	}

	f_free(pool->alloc, obuckets);
	pool->htab.buckets = nbuckets;
	pool->htab.bucket_count = ncount;

	if (LOG_ISS) {
		fprintf(stderr, "%s: %" PRIu64 " -> %" PRIu64 ", max_collisions %zu\n",
		    __func__, ocount, ncount, max_collisions);
		dump_tables(stderr, pool);
	}
	return true;
}

/* Save a state set and get a unique identifier that can refer
 * to it later. If there is more than one instance of the same
 * set of states, reuse the existing identifier. */
bool
interned_state_set_intern_set(struct interned_state_set_pool *pool,
	size_t state_count, const fsm_state_t *states,
	interned_state_set_id *result)
{
#if LOG_ISS > 2
	fprintf(stderr, "%s: state_count %zu\n", __func__, state_count);
#endif

#if EXPENSIVE_CHECKS
	/* input must be ordered */
	for (size_t i = 1; i < state_count; i++) {
		assert(states[i - 1] <= states[i]);
	}
#endif

	assert(state_count > 0);

	/* If there's only one state, return a singleton. */
	if (state_count == 1) {
		*result = states[0] | SINGLETON_BIT;
		return true;
	}

	/* otherwise, check the hash table */
	if (pool->htab.used > pool->htab.bucket_count/2) {
		if (!grow_htab(pool)) {
			return false;
		}
	}

	const uint64_t mask = pool->htab.bucket_count - 1;
	const uint64_t h = hash_state_ids(state_count, states);

	/*
	 * TODO: When state_count is > 1 these sets tend to partially
	 * overlap, so if reducing memory usage becomes more important
	 * than time, they could be compressed by interning state IDs
	 * pairwise: for example, [121 261 264 268 273 276 279] might
	 * become ,4 279 where:
	 *
	 * - ,0 -> 121 261
	 * - ,1 -> 264 268
	 * - ,2 -> 273 276
	 * - ,3 -> ,0 ,1
	 * - ,4 -> ,3 ,2
	 *
	 * and each pair is stored as uint32_t[2] and interned,
	 * where pair IDs have the msb set. fsm_state_t is expected
	 * to fit in 31 bits.
	 *
	 * As long as the pairwise chunking is done bottom-up (group
	 * all pairs of IDs, then all pairs of pairs, ...) it should
	 * also converge on the same top level ID, for constant
	 * comparison, and creating a new pair ID means the set has
	 * not been seen before. */

	uint32_t *dst_bucket = NULL;
	size_t probes = 0;
	for (uint64_t b_i = 0; b_i < pool->htab.bucket_count; b_i++) {
		uint32_t *b = &pool->htab.buckets[(h + b_i) & mask];
#if LOG_ISS > 1
		fprintf(stderr, "%s: htab[(0x%lx + %lu) & 0x%lx => %d\n",
		    __func__, h, b_i, mask, *b);
#endif
		if (*b == NO_ID) {
#if LOG_ISS > 3
			fprintf(stderr, "%s: empty bucket (%zd probes)\n", __func__, probes);
#endif
			dst_bucket = b;
			break;
		}

		const uint32_t id = *b;
		assert(id < pool->sets.used);
		struct issp_set *s = &pool->sets.sets[id];
		if (s->length != state_count) {
			probes++;
			continue;
		}


		assert(s->buf_offset + s->length <= pool->buf.used);
		const fsm_state_t *buf = &pool->buf.buf[s->buf_offset];

		if (0 == memcmp(states, buf, s->length * sizeof(buf[0]))) {
			*result = id;
#if LOG_ISS > 3
			fprintf(stderr, "%s: reused %u (%zd probes)\n", __func__, id, probes);
#endif
			return true;
		} else {
			probes++;
			continue;
		}
	}
	assert(dst_bucket != NULL);

#if LOG_ISS > 3
	fprintf(stderr, "%s: miss after %zd probes\n", __func__, probes);
#endif


#define VERIFY_HTAB_WITH_LINEAR_SERACH 0
#if VERIFY_HTAB_WITH_LINEAR_SERACH
	for (interned_state_set_id i = 0; i < pool->sets.used; i++) {
		struct issp_set *s = &pool->sets.sets[i];
		if (s->length == state_count &&
		    0 == memcmp(states,
			&pool->buf.buf[s->buf_offset],
			s->length * sizeof(states[0]))) {
			dump_tables(stderr, pool);
			fprintf(stderr, "%s: expected to find set %zu in htab but did not\n",
			    __func__, i);
			assert(!"missed in htab but hit in linear search");
		}
	}
#endif

	const interned_state_set_id dst = pool->sets.used;

	/* if not found, add to set list and buffer, growing as necessary */
	if (pool->sets.used == pool->sets.ceil) {
		const size_t nceil = 2*pool->sets.ceil;
		struct issp_set *nsets = f_realloc(pool->alloc,
		    pool->sets.sets, nceil * sizeof(nsets[0]));
		if (nsets == NULL) {
			return false;
		}
		pool->sets.ceil = nceil;
		pool->sets.sets = nsets;
	}
	if (pool->buf.used + state_count > pool->buf.ceil) {
		size_t nceil = 2*pool->buf.ceil;
		while (nceil < pool->buf.used + state_count) {
			nceil *= 2;
		}
		fsm_state_t *nbuf = f_realloc(pool->alloc,
		    pool->buf.buf, nceil * sizeof(nbuf[0]));
		if (nbuf == NULL) {
			return false;
		}
		pool->buf.ceil = nceil;
		pool->buf.buf = nbuf;
	}

	struct issp_set *s = &pool->sets.sets[dst];
	s->ss = NULL;
	s->buf_offset = pool->buf.used;
	s->length = state_count;

	memcpy(&pool->buf.buf[s->buf_offset],
	    states,
	    s->length * sizeof(states[0]));
	pool->buf.used += state_count;

	*result = dst;
	*dst_bucket = dst;
	pool->htab.used++;
	pool->sets.used++;

#if LOG_ISS
	fprintf(stderr, "%s: interned new %ld with %zd states, htab now using %zu/%zu\n",
	    __func__, *result, state_count, pool->htab.used, pool->htab.bucket_count);
#endif
	return true;
}

typedef void
iter_iss_cb(fsm_state_t s, void *opaque);

static void
iter_interned_state_set(const struct interned_state_set_pool *pool,
    interned_state_set_id iss_id, iter_iss_cb *cb, void *opaque)
{
	assert(pool != NULL);
	if (IS_SINGLETON(iss_id)) {
		cb(MASK_SINGLETON(iss_id), opaque);
		return;
	}

	assert(iss_id < pool->sets.used);
	const struct issp_set *s = &pool->sets.sets[iss_id];

#if LOG_ISS
	fprintf(stderr, "%s: iss_id %ld => s->buf_offset %zu, length %zu\n",
	    __func__, iss_id, s->buf_offset, s->length);
#endif

	assert(s->buf_offset + s->length <= pool->buf.used);
	const fsm_state_t *buf = &pool->buf.buf[s->buf_offset];

	for (size_t i = 0; i < s->length; i++) {
		cb(buf[i], opaque);
	}
}

struct mk_state_set_env {
	bool ok;
	const struct fsm_alloc *alloc;
	struct state_set *dst;
};

static void
mk_state_set_cb(fsm_state_t s, void *opaque)
{
	struct mk_state_set_env *env = opaque;
	if (!state_set_add(&env->dst, env->alloc, s)) {
		env->ok = false;
	}
}

/* Get the state_set associated with an interned ID.
 * If the state_set has already been built, return the saved instance. */
struct state_set *
interned_state_set_get_state_set(struct interned_state_set_pool *pool,
    interned_state_set_id iss_id)
{
	if (IS_SINGLETON(iss_id)) {
		struct state_set *dst = NULL;
		const fsm_state_t s = MASK_SINGLETON(iss_id);
		if (!state_set_add(&dst, pool->alloc, s)) {
			return false;
		}
		return dst;	/* also a singleton */
	}

	assert(iss_id < pool->sets.used);
	struct issp_set *s = &pool->sets.sets[iss_id];

	if (s->ss == NULL) {
		struct mk_state_set_env env = {
			.alloc = pool->alloc,
			.ok = true,
			.dst = NULL,
		};
		iter_interned_state_set(pool, iss_id, mk_state_set_cb, &env);

		if (!env.ok) {
			state_set_free(env.dst);
			env.dst = NULL;
		}
		s->ss = env.dst;
	}

	return s->ss;
}

static void
dump_state_set_cb(fsm_state_t s, void *opaque)
{
	FILE *f = opaque;
	fprintf(f, " %d", s);
}

/* Dump the list of states associated with a set ID. For debugging. */
void
interned_state_set_dump(FILE *f, const struct interned_state_set_pool *pool,
    interned_state_set_id set_id)
{
	iter_interned_state_set(pool, set_id, dump_state_set_cb, f);
}
