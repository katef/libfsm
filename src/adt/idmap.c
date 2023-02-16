/*
 * Copyright 2021 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "adt/idmap.h"

#include "adt/alloc.h"
#include "adt/hash.h"
#include "adt/u64bitset.h"

#include <stdint.h>
#include <assert.h>
#include <stdio.h>

#define NO_STATE ((fsm_state_t)-1)

#define DEF_BUCKET_COUNT 4

struct idmap {
	const struct fsm_alloc *alloc;
	unsigned bucket_count;
	unsigned buckets_used;

	/* All buckets' values are assumed to be large
	 * enough to store this value, and they will all
	 * grow as necessary. */
	unsigned max_value;

	/* Basic linear-probing, add-only hash table. */
	struct idmap_bucket {
		fsm_state_t state; /* Key. NO_STATE when empty. */

		/* values[] is always either NULL or has at least
		 * max_value + 1 bits; all grow on demand. */
		uint64_t *values;
	} *buckets;
};

static unsigned
value_words(unsigned max_value) {
	if (max_value == 0) {
		/* Still allocate one word, for storing 0. */
		return 1;
	} else {
		return u64bitset_words(max_value);
	}
}

struct idmap *
idmap_new(const struct fsm_alloc *alloc)
{
	struct idmap *res = NULL;
	struct idmap_bucket *buckets = NULL;

	res = f_malloc(alloc, sizeof(*res));
	if (res == NULL) {
		goto cleanup;
	}

	buckets = f_calloc(alloc,
	    DEF_BUCKET_COUNT, sizeof(buckets[0]));
	if (buckets == NULL) {
		goto cleanup;
	}

	for (size_t i = 0; i < DEF_BUCKET_COUNT; i++) {
		buckets[i].state = NO_STATE;
	}

	res->alloc = alloc;
	res->buckets_used = 0;
        res->bucket_count = DEF_BUCKET_COUNT;
	res->max_value = 0;
	res->buckets = buckets;

	return res;

cleanup:
	f_free(alloc, res);
	f_free(alloc, buckets);
	return NULL;
}

void
idmap_free(struct idmap *m)
{
	if (m == NULL) {
		return;
	}

	for (size_t i = 0; i < m->bucket_count; i++) {
		if (m->buckets[i].state == NO_STATE) {
			continue;
		}
		f_free(m->alloc, m->buckets[i].values);
	}

	f_free(m->alloc, m->buckets);
	f_free(m->alloc, m);
}

static int
grow_bucket_values(struct idmap *m, unsigned old_words, unsigned new_words)
{
	assert(new_words > old_words);

	for (size_t b_i = 0; b_i < m->bucket_count; b_i++) {
		struct idmap_bucket *b = &m->buckets[b_i];
		if (b->state == NO_STATE) {
			assert(b->values == NULL);
			continue;
		}

		uint64_t *nv = f_calloc(m->alloc,
		    new_words, sizeof(nv[0]));
		if (nv == NULL) {
			return 0;
		}

		for (size_t w_i = 0; w_i < old_words; w_i++) {
			nv[w_i] = b->values[w_i];
		}
		f_free(m->alloc, b->values);
		b->values = nv;
	}
	return 1;
}

static int
grow_buckets(struct idmap *m)
{
	const size_t ocount = m->bucket_count;
	const size_t ncount = 2*ocount;
	assert(ncount > m->bucket_count);

	struct idmap_bucket *nbuckets = f_calloc(m->alloc,
	    ncount, sizeof(nbuckets[0]));
	if (nbuckets == NULL) {
		return 0;
	}
	for (size_t nb_i = 0; nb_i < ncount; nb_i++) {
		nbuckets[nb_i].state = NO_STATE;
	}

	const size_t nmask = ncount - 1;

	for (size_t ob_i = 0; ob_i < ocount; ob_i++) {
		const struct idmap_bucket *ob = &m->buckets[ob_i];
		if (ob->state == NO_STATE) {
			continue;
		}

		const uint64_t h = hash_id(ob->state);
		for (size_t nb_i = 0; nb_i < ncount; nb_i++) {
			struct idmap_bucket *nb = &nbuckets[(h + nb_i) & nmask];
			if (nb->state == NO_STATE) {
				nb->state = ob->state;
				nb->values = ob->values;
				break;
			} else {
				assert(nb->state != ob->state);
				/* collision */
				continue;
			}
		}
	}

	f_free(m->alloc, m->buckets);

	m->buckets = nbuckets;
	m->bucket_count = ncount;

	return 1;
}

int
idmap_set(struct idmap *m, fsm_state_t state_id,
    unsigned value)
{
	assert(state_id != NO_STATE);

	const uint64_t h = hash_id(state_id);
	if (value > m->max_value) {
		const unsigned ovw = value_words(m->max_value);
		const unsigned nvw = value_words(value);
		/* If this value won't fit in the existing value
		 * arrays, then grow them all. We do not track the
		 * number of bits in each individual array. */
		if (nvw > ovw && !grow_bucket_values(m, ovw, nvw)) {
			return 0;
		}
		m->max_value = value;
	}

	assert(m->max_value >= value);

	if (m->buckets_used >= m->bucket_count/2) {
		if (!grow_buckets(m)) {
			return 0;
		}
	}

	const uint64_t mask = m->bucket_count - 1;
	for (size_t b_i = 0; b_i < m->bucket_count; b_i++) {
		struct idmap_bucket *b = &m->buckets[(h + b_i) & mask];
		if (b->state == state_id) {
			assert(b->values != NULL);
			u64bitset_set(b->values, value);
			return 1;
		} else if (b->state == NO_STATE) {
			b->state = state_id;
			assert(b->values == NULL);

			const unsigned vw = value_words(m->max_value);
			b->values = f_calloc(m->alloc,
			    vw, sizeof(b->values[0]));
			if (b->values == NULL) {
				return 0;
			}
			m->buckets_used++;

			u64bitset_set(b->values, value);
			return 1;
		} else {
			continue; /* collision */
		}

	}

	assert(!"unreachable");
	return 0;
}

static const struct idmap_bucket *
get_bucket(const struct idmap *m, fsm_state_t state_id)
{
	const uint64_t h = hash_id(state_id);
	const uint64_t mask = m->bucket_count - 1;
	for (size_t b_i = 0; b_i < m->bucket_count; b_i++) {
		const struct idmap_bucket *b = &m->buckets[(h + b_i) & mask];
		if (b->state == NO_STATE) {
			return NULL;
		} else if (b->state == state_id) {
			return b;
		}
	}

	return NULL;
}

size_t
idmap_get_value_count(const struct idmap *m, fsm_state_t state_id)
{
	const struct idmap_bucket *b = get_bucket(m, state_id);
	if (b == NULL) {
		return 0;
	}
	assert(b->values != NULL);

	size_t res = 0;
	const size_t words = value_words(m->max_value);
	for (size_t w_i = 0; w_i < words; w_i++) {
		const uint64_t w = b->values[w_i];
		/* This could use popcount64(w). */
		if (w == 0) {
			continue;
		}
		for (uint64_t bit = 1; bit; bit <<= 1) {
			if (w & bit) {
				res++;
			}
		}
	}

	return res;
}

int
idmap_get(const struct idmap *m, fsm_state_t state_id,
    size_t buf_size, unsigned *buf, size_t *written)
{
	const struct idmap_bucket *b = get_bucket(m, state_id);
	if (b == NULL) {
		if (written != NULL) {
			*written = 0;
		}
		return 1;
	}

	size_t buf_offset = 0;
	const size_t words = value_words(m->max_value);
	for (size_t w_i = 0; w_i < words; w_i++) {
		const uint64_t w = b->values[w_i];
		if (w == 0) {
			continue;
		}

		for (uint64_t b_i = 0; b_i < 64; b_i++) {
			if (w & ((uint64_t)1 << b_i)) {
				if (buf_offset * sizeof(buf[0]) >= buf_size) {
					return 0;
				}
				buf[buf_offset] = 64*w_i + b_i;
				buf_offset++;
			}
		}
	}

	if (written != NULL) {
		*written = buf_offset;
	}
	return 1;
}

void
idmap_iter(const struct idmap *m,
    idmap_iter_fun *cb, void *opaque)
{
	const size_t words = value_words(m->max_value);

	for (size_t b_i = 0; b_i < m->bucket_count; b_i++) {
		const struct idmap_bucket *b = &m->buckets[b_i];
		if (b->state == NO_STATE) {
			continue;
		}

		for (size_t w_i = 0; w_i < words; w_i++) {
			const uint64_t w = b->values[w_i];
			if (w == 0) {
				continue;
			}
			for (uint64_t b_i = 0; b_i < 64; b_i++) {
				if (w & ((uint64_t)1 << b_i)) {
					const unsigned v = 64*w_i + b_i;
					cb(b->state, v, opaque);
				}
			}
		}
	}
}

void
idmap_iter_for_state(const struct idmap *m, fsm_state_t state_id,
    idmap_iter_fun *cb, void *opaque)
{
	const size_t words = value_words(m->max_value);
	const struct idmap_bucket *b = get_bucket(m, state_id);
	if (b == NULL) {
		return;
	}

	for (size_t w_i = 0; w_i < words; w_i++) {
		const uint64_t w = b->values[w_i];
		if (w == 0) {
			continue;
		}
		/* if N contiguous bits are all zero, skip them all at once */
#define BLOCK_BITS 16
		uint64_t block = ((uint64_t)1 << BLOCK_BITS) - 1;
		size_t block_count = 0;

		uint64_t b_i = 0;
		while (b_i < 64) {
			if ((w & block) == 0) {
				block <<= BLOCK_BITS;
				b_i += BLOCK_BITS;
				continue;
			}

			if (w & ((uint64_t)1 << b_i)) {
				const unsigned v = 64*w_i + b_i;
				cb(b->state, v, opaque);
				block_count++;
			}
			b_i++;
			block <<= 1;
		}

#define CHECK 0
#if CHECK
		size_t check_count = 0;
		for (uint64_t b_i = 0; b_i < 64; b_i++) {
			if (w & ((uint64_t)1 << b_i)) {
				check_count++;
			}
		}
		assert(block_count == check_count);
#endif
	}
}
