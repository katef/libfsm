/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <stddef.h>
#include <inttypes.h>

#include "endids_internal.h"

#define LOG_ENDIDS 0

/* flags to enable or disable some expensive checks */
#define ENDIDS_EXPENSIVE_CHECK_SORTED_ORDER 0
#define ENDIDS_EXPENSIVE_CHECK_BISECTION    0

/* Convenience macros to avoid #if LOG_ENDIDS > n ... #endif pairs that disrupt
 * the flow
 * 
 * Note: __VA_ARGS__ requires C99
 */
#if LOG_ENDIDS > 2
#	define LOG_2(...) fprintf(stderr, __VA_ARGS__)
#	define DBG_2(expr)    do { (expr); } while (0)
#else
#	define LOG_2(...) do {} while(0)
#	define DBG_2(expr)    do {} while (0)
#endif

#if LOG_ENDIDS > 3
#	define LOG_3(...) fprintf(stderr, __VA_ARGS__)
#	define DBG_3(expr)    do { (expr); } while (0)
#else
#	define LOG_3(...) do {} while(0)
#	define DBG_3(expr)    do {} while (0)
#endif

#if LOG_ENDIDS > 4
#	define LOG_4(...) fprintf(stderr, __VA_ARGS__)
#	define DBG_4(expr)    do { (expr); } while (0)
#else
#	define LOG_4(...) do {} while(0)
#	define DBG_4(expr)    do {} while (0)
#endif

static void
dump_buckets(const char *tag, const struct endid_info *ei)
{
#if LOG_ENDIDS > 3
	size_t j;
	for (j = 0; j < ei->bucket_count; j++) {
		struct endid_info_bucket *b = &ei->buckets[j];
		fprintf(stderr, "%s[%lu/%u]: %d (on %p)\n",
		    tag,
		    j, ei->bucket_count, b->state, (void *)ei->buckets);
	}
#else
	(void)tag;
	(void)ei;
#endif
}

int
fsm_setendid(struct fsm *fsm, fsm_end_id_t id)
{
	fsm_state_t i;

	/* for every end state */
	for (i = 0; i < fsm->statecount; i++) {
		if (fsm_isend(fsm, i)) {
			enum fsm_endid_set_res sres;
#if LOG_ENDIDS > 3
			fprintf(stderr, "fsm_setendid: setting id %u on state %d\n",
			    id, i);
#endif
			sres = fsm_endid_set(fsm, i, id);
			if (sres == FSM_ENDID_SET_ERROR_ALLOC_FAIL) {
				return 0;
			}
		}
	}

	return 1;
}

enum fsm_getendids_res
fsm_getendids(const struct fsm *fsm, fsm_state_t end_state,
    size_t id_buf_count, fsm_end_id_t *id_buf,
    size_t *ids_written)
{
	return fsm_endid_get(fsm, end_state,
	    id_buf_count, id_buf, ids_written);
}

size_t
fsm_getendidcount(const struct fsm *fsm, fsm_state_t end_state)
{
	return fsm_endid_count(fsm, end_state);
}

int
fsm_endid_init(struct fsm *fsm)
{
	struct endid_info_bucket *buckets = NULL;
	size_t i;
	struct endid_info *res = f_calloc(fsm->opt->alloc, 1, sizeof(*res));
	if (res == NULL) {
		return 0;
	}

	buckets = f_malloc(fsm->opt->alloc,
	    DEF_BUCKET_COUNT * sizeof(buckets[0]));
	if (buckets == NULL) {
		f_free(fsm->opt->alloc, res);
		return 0;
	}

	/* initialize the hash table's buckets to empty */
	for (i = 0; i < DEF_BUCKET_COUNT; i++) {
		buckets[i].state = BUCKET_NO_STATE;
	}

	res->buckets = buckets;
	res->bucket_count = DEF_BUCKET_COUNT;
	res->buckets_used = 0;

	fsm->endid_info = res;

#if LOG_ENDIDS > 3
	fprintf(stderr, "fsm_endid_init: initialized with %u buckets\n",
	    res->bucket_count);
#endif

	return 1;
}

void
fsm_endid_free(struct fsm *fsm)
{
	size_t i;
	if (fsm == NULL || fsm->endid_info == NULL) {
		return;
	}

	for (i = 0; i < fsm->endid_info->bucket_count; i++) {
		struct endid_info_bucket *b = &fsm->endid_info->buckets[i];
		if (b->state == BUCKET_NO_STATE) {
			continue;
		}
		f_free(fsm->opt->alloc, fsm->endid_info->buckets[i].ids);
	}
	f_free(fsm->opt->alloc, fsm->endid_info->buckets);
	f_free(fsm->opt->alloc, fsm->endid_info);
}

static int
grow_endid_buckets(const struct fsm_alloc *alloc, struct endid_info *info)
{
	size_t old_i, new_i;
	struct endid_info_bucket *old_buckets, *new_buckets;
	size_t new_count, new_used = 0, new_mask;
	const size_t old_count = info->bucket_count;

	new_count = 2*info->bucket_count;
	assert(new_count > old_count);
	old_buckets = info->buckets;
	new_mask = new_count - 1;
	assert((new_mask & new_count) == 0); /* power of 2 */

#if LOG_ENDIDS > 3
	fprintf(stderr, "grow_endid_buckets: growing from %lu to %lu buckets\n",
	    old_count, new_count);
#endif

	new_buckets = f_malloc(alloc, new_count * sizeof(new_buckets[0]));
	if (new_buckets == NULL) {
		return 0;
	}

	for (new_i = 0; new_i < new_count; new_i++) {
		new_buckets[new_i].state = BUCKET_NO_STATE;
	}

	for (old_i = 0; old_i < old_count; old_i++) {
		struct endid_info_bucket *src_b = &old_buckets[old_i];
		struct endid_info_bucket *dst_b;
		uint64_t hash;
		int copied = 0;
		if (src_b->state == BUCKET_NO_STATE) {
			continue;
		}
		hash = hash_id(src_b->state);
		for (new_i = 0; new_i < new_count; new_i++) {
			dst_b = &new_buckets[(hash + new_i) & new_mask];
			if (dst_b->state == BUCKET_NO_STATE) {
				dst_b->state = src_b->state;
				dst_b->ids = src_b->ids;
				copied = 1;
				new_used++;
				break;
			}
		}
		assert(copied);
	}
	assert(new_used == info->buckets_used);

	f_free(alloc, info->buckets);
	info->buckets = new_buckets;
	info->bucket_count = new_count;
	return 1;
}

static struct endid_info_bucket *
endid_find_bucket(struct fsm *fsm, fsm_state_t state)
{
	struct endid_info *ei = NULL;
	int has_grown = 0;
	uint64_t hash;
	size_t i;
	uint64_t mask;

	assert(fsm != NULL);
	ei = fsm->endid_info;
	assert(ei != NULL);

rehash:

	hash = hash_id(state);
	mask = ei->bucket_count - 1;
	assert((mask & ei->bucket_count) == 0); /* power of 2 */

	LOG_2("endid_find_bucket: state %d -> hash %" PRIx64 "\n", state, hash);

	for (i = 0; i < ei->bucket_count; i++) {
		const size_t b_i = (hash + i) & mask;
		struct endid_info_bucket *b = &ei->buckets[b_i];

		LOG_2("fsm_endid_set[%lu/%u]: %d for state %d\n",
		    b_i, ei->bucket_count, b->state, state);

		if (b->state == state) {
			return b;
		}

		else if (b->state == BUCKET_NO_STATE) { /* empty */

			LOG_2("fsm_endid_set: setting empty bucket %lu\n", b_i);

			if (ei->buckets_used == ei->bucket_count / 2) {
				assert(!has_grown); /* should only happen once */

				LOG_2("  -- growing buckets [%u/%u used], then rehashing\n",
				    ei->buckets_used, ei->bucket_count);

				if (!grow_endid_buckets(fsm->opt->alloc, ei)) {
					return NULL;
				}
				has_grown = 1;
				/* At this point, we need to re-hash, since
				 * the mask has changed and the collisions
				 * may be different. */
				goto rehash;
			}

			return b;

		} else {   /* collision */
			LOG_4("fsm_endid_set: collision, continuing\n");
			continue;
		}
	}

	assert(!"unreachable: full but not resizing");
	return NULL;
}

static int
check_ids_sorted_and_unique(const fsm_end_id_t *ids, size_t n);

/* Bisects a sorted list similar to Python's bisect.bisect_left function to return
 * an index that can be used for insertion.
 *
 * This function assumes that the items in the array are unique.
 *
 * Returns the entry k of ids such that:
 *     0 <= k <= n
 *     if 0 < k < n, then: ids[k-1] < itm <= ids[k]
 *     if k == 0, then: itm <= ids[0]
 *     if k == n, then: ids[n-1] < itm
 *
 * Note that ids must be unique and sorted in ascending order:
 *     if i < j, then ids[i] < ids[j]
 */
static size_t
bisect_left_sorted_ids(fsm_end_id_t itm, const fsm_end_id_t *ids, size_t n)
{
	ptrdiff_t lo,hi;

	assert(check_ids_sorted_and_unique(ids, n));

	/* fast paths for empty, prepend or append */
	if (n == 0) {
		return 0;
	}

	if (itm < ids[0]) {
		return 0;
	}

	if (itm > ids[n-1]) {
		return n;
	}

	/* binary search */
	lo=0;
	hi=n-1;
	while (lo <= hi) {
		ptrdiff_t mid = (lo+hi)/2;

		if (itm == ids[mid]) {
			return mid;
		}

		if (itm < ids[mid]) {
			hi = mid-1;
		}

		if (itm > ids[mid]) {
			lo = mid+1;
		}
	}

	assert(lo == hi+1);
	assert(ids[hi] < itm);
	assert(ids[lo] >= itm);

#if ENDIDS_EXPENSIVE_CHECK_BISECTION
	{
		size_t i;

		assert(lo >= 0);
		for (i=0; i < n; i++) {
			if (i < (size_t)lo) {
				assert(ids[i] < itm);
			} else {
				assert(ids[i] >= itm);
			}
		}
	}
#endif /* ENDIDS_EXPENSIVE_CHECK_BISECTION */

	return lo;
}

static int
check_ids_sorted_and_unique(const fsm_end_id_t *ids, size_t n)
{
	(void) ids;
	(void) n;

#if ENDIDS_EXPENSIVE_CHECK_SORTED_ORDER
	fprintf(stderr, " --> checking: ids sorted and unique\n");
	size_t i;
	for (i=1; i < n; i++) {
		if (ids[i] < ids[i-1]) {
			return 0;
		}
	}
#endif /* ENDIDS_EXPENSIVE_CHECK_SORTED_ORDER */

	return 1;
}

static struct end_info_ids *
allocate_ids(const struct fsm *fsm, struct end_info_ids *prev, size_t n)
{
	struct end_info_ids *ids;
	size_t id_alloc_size;

	assert(n > 0);

	id_alloc_size = sizeof(*ids) + (n - 1) * sizeof(ids->ids[0]);
	return f_realloc(fsm->opt->alloc, prev, id_alloc_size);
}

enum fsm_endid_set_res
fsm_endid_set(struct fsm *fsm,
    fsm_state_t state, fsm_end_id_t id)
{
	struct endid_info *ei = NULL;

	assert(fsm != NULL);
	ei = fsm->endid_info;
	assert(ei != NULL);

	struct endid_info_bucket *b = endid_find_bucket(fsm, state);
	if (b == NULL) {
		return FSM_ENDID_SET_ERROR_ALLOC_FAIL;
	}

	LOG_2("fsm_endid_set: state %d, bucket %p\n", state, (void *)b);

	if (b->state == BUCKET_NO_STATE) { 
		struct end_info_ids *ids;

		ids = allocate_ids(fsm, NULL, DEF_BUCKET_ID_COUNT);
		if (ids == NULL) {
			return FSM_ENDID_SET_ERROR_ALLOC_FAIL;
		}

		ids->ids[0] = id;
		ids->count = 1;
		ids->ceil = DEF_BUCKET_ID_COUNT;

		b->state = state;
		b->ids = ids;
		ei->buckets_used++;

		return FSM_ENDID_SET_ADDED;
	} else if (b->state == state) {
		size_t ind;

		LOG_2("fsm_endid_set: appending to bucket %zd's IDs, [%u/%u used]\n",
			(b - &ei->buckets[0]), b->ids->count, b->ids->ceil);;

		if (b->ids->count == b->ids->ceil) { /* grow? */
			struct end_info_ids *nids;
			const size_t nceil = 2*b->ids->ceil;

			assert(nceil > b->ids->ceil);

			LOG_2("fsm_endid_set: growing id array %u -> %zu\n", b->ids->ceil, nceil);

			nids = allocate_ids(fsm, b->ids, nceil);
			if (nids == NULL) {
				return FSM_ENDID_SET_ERROR_ALLOC_FAIL; /* alloc fail */
			}
			nids->ceil = nceil;
			b->ids = nids;
		}

		/* can't use bsearch here.  bsearch returns NULL if the item is not found, 
		 * and what we really want is the item that's the least upper bound:
		 *
		 *     min{ind, id <= b->ids->ids[ind]}
		 */
		ind = bisect_left_sorted_ids(id, b->ids->ids, b->ids->count);
		assert(ind <= b->ids->count);

		if (ind == b->ids->count) {
			/* append */
			b->ids->ids[b->ids->count] = id;
			b->ids->count++;
		} else if (b->ids->ids[ind] == id) {
			/* already present, our work is done! */
			LOG_2("fsm_endid_set: already present, skipping\n");
			return FSM_ENDID_SET_ALREADY_PRESENT;
		} else {
			/* need to shift items up to make room for id */
			memmove(&b->ids->ids[ind+1], &b->ids->ids[ind], 
				(b->ids->count - ind) * sizeof b->ids->ids[0]);
			b->ids->ids[ind] = id;
			b->ids->count++;
		}

		(void)check_ids_sorted_and_unique;
		assert(check_ids_sorted_and_unique(b->ids->ids, b->ids->count));

		LOG_3("fsm_endid_set: wrote %d at %d/%d\n", id, b->ids->count - 1, b->ids->ceil);
                (void)dump_buckets;
		DBG_3(dump_buckets("set_dump", ei));

		return FSM_ENDID_SET_ADDED;
	} else {
	    assert(!"unreachable: endid_find_bucket failed");
	    return FSM_ENDID_SET_ERROR_ALLOC_FAIL;
	}
}

static int
cmp_endids(const void *pa, const void *pb)
{
	const fsm_end_id_t *a = pa;
	const fsm_end_id_t *b = pb;

	if (*a < *b) {
		return -1;
	}

	if (*a > *b) {
		return 1;
	}

	return 0;
}

enum fsm_endid_set_res
fsm_endid_set_bulk(struct fsm *fsm,
    fsm_state_t state, size_t num_ids, const fsm_end_id_t *ids, enum fsm_endid_bulk_op op)
{
	struct endid_info *ei = NULL;
	struct endid_info_bucket *b;

	assert(fsm != NULL);

	ei = fsm->endid_info;
	assert(ei != NULL);

	b = endid_find_bucket(fsm, state);
	if (b == NULL) {
		return FSM_ENDID_SET_ERROR_ALLOC_FAIL;
	}

	if (b->state == state) {
		size_t prev_count;
		size_t total_count;

		assert(b->ids != NULL);

		prev_count = (op == FSM_ENDID_BULK_REPLACE) ? 0 : b->ids->count;
		total_count = num_ids + prev_count;

		if (total_count > b->ids->ceil) {
			struct end_info_ids *new_ids;
			size_t new_ceil = 2*b->ids->ceil;

			/* TODO: use log2 */
			while (new_ceil < total_count && new_ceil <= SIZE_MAX/2) {
				new_ceil *= 2;
			}

			if (new_ceil < total_count) {
				/* num_ids is too large? */
				return FSM_ENDID_SET_ERROR_ALLOC_FAIL;
			}

			new_ids = allocate_ids(fsm, b->ids, new_ceil);
			if (new_ids == NULL) {
				return FSM_ENDID_SET_ERROR_ALLOC_FAIL;
			}

			b->ids = new_ids;
			b->ids->ceil = new_ceil;
		}

		assert(total_count <= b->ids->ceil);
		assert(total_count == num_ids + prev_count);

		if (op == FSM_ENDID_BULK_REPLACE) {
			assert(prev_count == 0);
			assert(total_count == num_ids);
		}

		memcpy(&b->ids->ids[prev_count], &ids[0], num_ids * sizeof ids[0]);
		b->ids->count = total_count;
	} else {
		struct end_info_ids *new_ids;
		size_t n;

		assert(b->state == BUCKET_NO_STATE);

		n = DEF_BUCKET_ID_COUNT;
		while (n < num_ids && n < SIZE_MAX/2) {
			n *= 2;
		}

		if (n < num_ids) {
			/* num_ids is too large? */
			return FSM_ENDID_SET_ERROR_ALLOC_FAIL;
		}

		new_ids = allocate_ids(fsm, NULL, n);
		if (new_ids == NULL) {
			return FSM_ENDID_SET_ERROR_ALLOC_FAIL;
		}

		memcpy(&new_ids->ids[0], &ids[0], num_ids * sizeof ids[0]);
		new_ids->count = num_ids;
		new_ids->ceil = n;

		b->state = state;
		b->ids = new_ids;
		ei->buckets_used++;
	}

	// sort and remove any duplicates
	qsort(&b->ids->ids[0], b->ids->count, sizeof b->ids->ids[0], cmp_endids);
	{
		size_t i,j,n;

		fsm_end_id_t *ids = b->ids->ids;
		n = b->ids->count;
		for (i=1, j=1; i < n; i++) {
			LOG_3("i = %zu, j = %zu, ids[i] = %u, ids[j] = %u\n", i,j,ids[i],ids[j]);
			if (ids[i]  != ids[i-1]) {
				if (i != j) {
					ids[j] = ids[i];
				}
				j++;
			} else {
				LOG_2("duplicate ids[%zu] == ids[%zu] = %u\n", i, i-1, ids[i]);
			}
		}

		LOG_2("removed id duplicates, %zu starting entries, %zu after duplicates removed\n",
			n, j);

		b->ids->count = j;
	}

	return FSM_ENDID_SET_ADDED;
}

int
fsm_mapendids(struct fsm *fsm,
	int (*remap)(fsm_state_t state, size_t num_ids, fsm_end_id_t *endids, size_t *num_written, void *opaque),
	void *opaque)
{
	size_t i;
	const struct endid_info *ei = NULL;

	assert(fsm != NULL);
	ei = fsm->endid_info;
	assert(ei != NULL);

	for (i = 0; i < ei->bucket_count; i++) {
		struct endid_info_bucket *b = &ei->buckets[i];

		if (b->state != BUCKET_NO_STATE) {
			int ret;
			size_t nwritten = 0;

			ret = remap(b->state, b->ids->count, b->ids->ids, &nwritten, opaque);
			if (nwritten > 0) {
				assert(nwritten <= b->ids->count);

				if (nwritten > 1) {
					size_t j,k;
					qsort(&b->ids->ids[0], nwritten, sizeof b->ids->ids[0], cmp_endids);

					/* check for any duplicates and remove them */
					for (j = 1, k = 1; j < nwritten; j++) {
						if (b->ids->ids[j] != b->ids->ids[j-1]) {
							if (j != k) {
								b->ids->ids[k] = b->ids->ids[j];
							}
							k++;
						}
					}

					nwritten = k;
				}

				b->ids->count = nwritten;
			}

			if (!ret) {
				return 0;
			}
		}
	}

	return 1;
}

size_t
fsm_endid_count(const struct fsm *fsm,
    fsm_state_t state)
{
	const struct endid_info *ei = NULL;
	size_t i;

	uint64_t hash = hash_id(state);
	uint64_t mask;

	assert(fsm != NULL);
	ei = fsm->endid_info;
	assert(ei != NULL);

	mask = ei->bucket_count - 1;
	/* bucket count is a power of 2 */
	assert((mask & ei->bucket_count) == 0);

	for (i = 0; i < ei->bucket_count; i++) {
		const size_t b_i = (hash + i) & mask;
		struct endid_info_bucket *b = &ei->buckets[b_i];

		if (b->state == BUCKET_NO_STATE) {
			return 0; /* not found */
		} else if (b->state == state) {
			return b->ids->count;
		} else {	/* collision */
			continue;
		}
	}
	assert(!"unreachable");
	return 0;
}

enum fsm_getendids_res
fsm_endid_get(const struct fsm *fsm, fsm_state_t end_state,
    size_t id_buf_count, fsm_end_id_t *id_buf,
    size_t *ids_written)
{
	size_t i;
	size_t written = 0;
	const struct endid_info *ei = NULL;

	uint64_t hash = hash_id(end_state);
	uint64_t mask;

	(void)written;

	assert(fsm != NULL);
	ei = fsm->endid_info;
	assert(ei != NULL);

	assert(id_buf != NULL);
	assert(ids_written != NULL);

	mask = ei->bucket_count - 1;
	/* bucket count is a power of 2 */
	assert((mask & ei->bucket_count) == 0);

#if LOG_ENDIDS > 3
	dump_buckets("fsm_endid_get", ei);
#endif

	for (i = 0; i < ei->bucket_count; i++) {
		const size_t b_i = (hash + i) & mask;
		struct endid_info_bucket *b = &ei->buckets[b_i];
#if LOG_ENDIDS > 2
		fprintf(stderr, "fsm_endid_get[%lu/%u]: %d for end_state %d\n",
		    b_i, ei->bucket_count, b->state, end_state);
#endif
		if (b->state == BUCKET_NO_STATE) {
#if LOG_ENDIDS > 2
			fprintf(stderr, "fsm_endid_get: not found\n");
#endif
			*ids_written = 0; /* not found */
			return FSM_GETENDIDS_NOT_FOUND;
		} else if (b->state == end_state) {
			size_t id_i;
			if (b->ids->count > id_buf_count) {
#if LOG_ENDIDS > 2
				fprintf(stderr, "fsm_endid_get: insufficient space\n");
#endif
				return FSM_GETENDIDS_ERROR_INSUFFICIENT_SPACE;
			}
			for (id_i = 0; id_i < b->ids->count; id_i++) {
#if LOG_ENDIDS > 2
				fprintf(stderr, "fsm_endid_get: writing id[%zu]: %d\n", id_i, b->ids->ids[id_i]);
#endif
				id_buf[id_i] = b->ids->ids[id_i];
			}

			/* todo: could sort them here, if it matters. */
			*ids_written = b->ids->count;
			return FSM_GETENDIDS_FOUND;
		} else {	/* collision */
#if LOG_ENDIDS > 4
			fprintf(stderr, "fsm_endid_get: collision\n");
#endif
			continue;
		}
	}

	assert(!"unreachable");
	return FSM_GETENDIDS_NOT_FOUND;
}

struct carry_env {
	char tag;
	struct fsm *dst;
	fsm_state_t dst_state;
	int ok;
};

static int
carry_iter_cb(fsm_state_t state, fsm_end_id_t id, void *opaque)
{
	enum fsm_endid_set_res sres;
	struct carry_env *env = opaque;
	assert(env->tag == 'C');

	(void)state;

	sres = fsm_endid_set(env->dst, env->dst_state, id);
	if (sres == FSM_ENDID_SET_ERROR_ALLOC_FAIL) {
		env->ok = 0;
		return 0;
	}
	return 1;
}

int
fsm_endid_carry(const struct fsm *src_fsm, const struct state_set *src_set,
    struct fsm *dst_fsm, fsm_state_t dst_state)
{
	const int log_carry = 0;

#define ID_BUF_SIZE 64
	struct state_iter it;
	fsm_state_t s;

	/* These must be distinct, otherwise we can end up adding to
	 * the hash table while iterating over it, and resizing will
	 * lead to corruption. */
	assert(src_fsm != dst_fsm);

	if (log_carry) {
		fprintf(stderr, "==== fsm_endid_carry: before\n");
		fsm_endid_dump(stderr, src_fsm);
	}

	for (state_set_reset(src_set, &it); state_set_next(&it, &s); ) {
		struct carry_env env;
		env.tag = 'C';
		env.dst = dst_fsm;
		env.dst_state = dst_state;
		env.ok = 1;

		if (!fsm_isend(src_fsm, s)) {
			continue;
		}

		fsm_endid_iter_state(src_fsm, s, carry_iter_cb, &env);
		if (!env.ok) {
			return 0;
		}
	}

	if (log_carry) {
		fprintf(stderr, "==== fsm_endid_carry: after\n");
		fsm_endid_dump(stderr, dst_fsm);
	}

	return 1;
}

void
fsm_endid_iter(const struct fsm *fsm,
    fsm_endid_iter_cb *cb, void *opaque)
{
	size_t b_i;
	struct endid_info *ei = NULL;
	size_t bucket_count;

	assert(fsm != NULL);
	assert(cb != NULL);

	ei = fsm->endid_info;
	if (ei == NULL) {
		return;
	}

	bucket_count = ei->bucket_count;

	for (b_i = 0; b_i < bucket_count; b_i++) {
		struct endid_info_bucket *b = &ei->buckets[b_i];
		size_t id_i;
		size_t count;
		if (b->state == BUCKET_NO_STATE) {
			continue;
		}
		count = b->ids->count;

		for (id_i = 0; id_i < count; id_i++) {
			if (!cb(b->state, b->ids->ids[id_i], opaque)) {
				break;
			}

			/* The cb must not grow the hash table. */
			assert(bucket_count == ei->bucket_count);
			assert(b->ids->count == count);
		}
	}
}

int
fsm_endid_iter_bulk(const struct fsm *fsm,
    fsm_endid_iter_bulk_cb *cb, void *opaque)
{
	size_t b_i;
	struct endid_info *ei = NULL;
	size_t bucket_count;

	assert(fsm != NULL);
	assert(cb != NULL);

	ei = fsm->endid_info;
	if (ei == NULL) {
		return 1;
	}

	bucket_count = ei->bucket_count;

	for (b_i = 0; b_i < bucket_count; b_i++) {
		struct endid_info_bucket *b = &ei->buckets[b_i];
		size_t count;
		if (b->state == BUCKET_NO_STATE) {
			continue;
		}

		count = b->ids->count;

		if (!cb(b->state, &b->ids->ids[0], count, opaque)) {
			return 0;
		}

		/* The cb must not grow the hash table. */
		assert(bucket_count == ei->bucket_count);
		assert(b->ids->count == count);
	}

	return 1;
}

void
fsm_endid_iter_state(const struct fsm *fsm, fsm_state_t state,
    fsm_endid_iter_cb *cb, void *opaque)
{
	size_t id_i = 0;
	size_t b_i;
	size_t bucket_count;
	struct endid_info *ei = NULL;

	uint64_t hash;
	uint64_t mask;

	assert(fsm != NULL);
	assert(cb != NULL);

	ei = fsm->endid_info;
	if (ei == NULL) {
		return;
	}

	assert(state != BUCKET_NO_STATE);

	hash = hash_id(state);
	bucket_count = ei->bucket_count;
	mask = bucket_count - 1;
	assert((mask & bucket_count) == 0); /* power of 2 */

#if LOG_ENDIDS > 2
	fprintf(stderr, "fsm_endid_iter_state: state %d -> hash %" PRIx64 "\n",
	    state, hash);
#endif

#if LOG_ENDIDS > 3
	dump_buckets("fsm_endid_iter_state, before", ei);
#endif

	for (b_i = 0; b_i < bucket_count; b_i++) {
		struct endid_info_bucket *b = &ei->buckets[(hash + b_i) & mask];
#if LOG_ENDIDS > 2
		fprintf(stderr, "fsm_endid_iter_state: bucket [%" PRIu64 "/%ld]: %d\n",
		    (hash + b_i) & mask, bucket_count, b->state);
#endif

		if (b->state == BUCKET_NO_STATE) { /* unused bucket */
			return;	/* empty -> not found */
		} else if (b->state == state) {
			const size_t id_count = b->ids->count;
			while (id_i < id_count) {
#if LOG_ENDIDS > 2
				fprintf(stderr, "fsm_endid_iter_state[%d], ids[%ld] -> %d\n",
				    b->state, id_i, b->ids->ids[id_i]);
#endif
				if (!cb(b->state, b->ids->ids[id_i], opaque)) {
					return;
				}
				id_i++;

				/* The cb must not grow the hash table. */
				assert(bucket_count == ei->bucket_count);
				assert(b->ids->count == id_count);
			}
			break;
		} else {
			assert(b->state != state);
			continue;
		}
	}
}

struct dump_env {
	FILE *f;
};

static int
dump_cb(fsm_state_t state, const fsm_end_id_t id, void *opaque)
{
	struct dump_env *env = opaque;
	fprintf(env->f, "state[%u]: %u\n", state, id);
	return 1;
}

void
fsm_endid_dump(FILE *f, const struct fsm *fsm)
{
	struct dump_env env;

	env.f = f;

	fsm_endid_iter(fsm, dump_cb, &env);
}

static int
incr_remap(fsm_state_t state, size_t num_ids, fsm_end_id_t *endids, size_t *num_written, void *opaque)
{
	const int *delta_ptr;
	int delta;
	size_t i;

	(void)state;

	delta_ptr = opaque;
	delta = *delta_ptr;

	for (i=0; i < num_ids; i++) {
		endids[i] += delta;
	}

	*num_written = num_ids;

	return 1;
}

void
fsm_increndids(struct fsm * fsm, int delta)
{
	if (delta == 0) {
		/* nothing to do ... */
		return;
	}

	fsm_mapendids(fsm, incr_remap, &delta);
}

