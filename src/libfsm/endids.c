/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include "endids_internal.h"

#define LOG_ENDIDS 0

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

SUPPRESS_EXPECTED_UNSIGNED_INTEGER_OVERFLOW()
static unsigned long
hash_state(fsm_state_t state)
{
	return PHI32 * (state + 1);
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
	    res->bucket_count);
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
		unsigned hash;
		int copied = 0;
		if (src_b->state == BUCKET_NO_STATE) {
			continue;
		}
		hash = hash_state(src_b->state);
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

enum fsm_endid_set_res
fsm_endid_set(struct fsm *fsm,
    fsm_state_t state, fsm_end_id_t id)
{
	struct endid_info *ei = NULL;
	int has_grown = 0;
	unsigned hash;
	size_t i;
	unsigned mask;

	assert(fsm != NULL);
	ei = fsm->endid_info;
	assert(ei != NULL);

rehash:

	hash = hash_state(state);
	mask = ei->bucket_count - 1;
	assert((mask & ei->bucket_count) == 0); /* power of 2 */

#if LOG_ENDIDS > 2
	fprintf(stderr, "fsm_endid_set: state %d, id %d -> hash %x\n",
	    state, id, hash);
#endif

	for (i = 0; i < ei->bucket_count; i++) {
		const size_t b_i = (hash + i) & mask;
		struct endid_info_bucket *b = &ei->buckets[b_i];

#if LOG_ENDIDS > 2
		fprintf(stderr, "fsm_endid_set[%lu/%u]: %d for state %d\n",
		    b_i, ei->bucket_count, b->state, state);
#endif

		if (b->state == BUCKET_NO_STATE) { /* empty */
			struct end_info_ids *ids = NULL;
			const size_t id_alloc_size = sizeof(*ids)
			    + (DEF_BUCKET_ID_COUNT - 1) * sizeof(ids->ids[0]);

#if LOG_ENDIDS > 2
		fprintf(stderr, "fsm_endid_set: setting empty bucket %lu\n", b_i);
#endif
			if (ei->buckets_used == ei->bucket_count / 2) {
				assert(!has_grown); /* should only happen once */

#if LOG_ENDIDS > 2
				fprintf(stderr, "  -- growing buckets [%u/%u used], then rehashing\n",
				    ei->buckets_used, ei->bucket_count);
#endif
				if (!grow_endid_buckets(fsm->opt->alloc, ei)) {
					return FSM_ENDID_SET_ERROR_ALLOC_FAIL;
				}
				has_grown = 1;
				/* At this point, we need to re-hash, since
				 * the mask has changed and the collisions
				 * may be different. */
				goto rehash;
			}

			ids = f_malloc(fsm->opt->alloc, id_alloc_size);
			if (ids == NULL) {
				return FSM_ENDID_SET_ERROR_ALLOC_FAIL;
			}
			ids->ids[0] = id;
			ids->count = 1;
			ids->ceil = DEF_BUCKET_ID_COUNT;

			b->state = state;
			b->ids = ids;
			ei->buckets_used++;

#if LOG_ENDIDS > 3
			fprintf(stderr, "fsm_endid_set: [%lu] now %d (on %p)\n",
			    b_i, b->state, (void *)ei->buckets);
			dump_buckets("set_dump", ei);
#else
	(void)dump_buckets;
#endif
			return FSM_ENDID_SET_ADDED;
		} else if (b->state == state) {	   /* add if not present */
			size_t j;
#if LOG_ENDIDS > 2
			fprintf(stderr, "fsm_endid_set: appending to bucket %lu's IDs, [%u/%u used]\n",
			    b_i, b->ids->count, b->ids->ceil);
#endif

			if (b->ids->count == b->ids->ceil) { /* grow? */
				struct end_info_ids *nids;
				const size_t nceil = 2*b->ids->ceil;
				const size_t id_realloc_size = sizeof(*nids)
				    + (nceil - 1) * sizeof(nids->ids[0]);
				assert(nceil > b->ids->ceil);

#if LOG_ENDIDS > 2
			fprintf(stderr, "fsm_endid_set: growing id array %u -> %zu\n",
			    b->ids->ceil, nceil);
#endif
				nids = f_realloc(fsm->opt->alloc,
				    b->ids, id_realloc_size);
				if (nids == NULL) {
					return FSM_ENDID_SET_ERROR_ALLOC_FAIL; /* alloc fail */
				}
				nids->ceil = nceil;
				b->ids = nids;
			}

			/* This does not order the IDs. We could also
			 * bsearch and shift them down, etc. */
			for (j = 0; j < b->ids->count; j++) {
				if (b->ids->ids[j] == id) {
#if LOG_ENDIDS > 2
					fprintf(stderr, "fsm_endid_set: already present, skipping\n");
#endif
					return FSM_ENDID_SET_ALREADY_PRESENT;
				}
			}

			b->ids->ids[b->ids->count] = id;
			b->ids->count++;

#if LOG_ENDIDS > 3
			fprintf(stderr, "fsm_endid_set: wrote %d at %d/%d\n",
			    id, b->ids->count - 1, b->ids->ceil);
			dump_buckets("set_dump", ei);
#endif
			return FSM_ENDID_SET_ADDED;
		} else {			   /* collision */
#if LOG_ENDIDS > 4
			fprintf(stderr, "fsm_endid_set: collision, continuing\n");
#endif
			continue;
		}
	}

	assert(!"unreachable: full but not resizing");
	return FSM_ENDID_SET_ERROR_ALLOC_FAIL;
}

size_t
fsm_endid_count(const struct fsm *fsm,
    fsm_state_t state)
{
	const struct endid_info *ei = NULL;
	size_t i;

	unsigned hash = hash_state(state);
	unsigned mask;

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

	unsigned hash = hash_state(end_state);
	unsigned mask;

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
				fprintf(stderr, "fsm_endid_get: writing id[%zu]: %d\n", k, b->ids->ids[k]);
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

void
fsm_endid_iter_state(const struct fsm *fsm, fsm_state_t state,
    fsm_endid_iter_cb *cb, void *opaque)
{
	size_t id_i = 0;
	size_t b_i;
	size_t bucket_count;
	struct endid_info *ei = NULL;

	unsigned hash;
	unsigned mask;

	assert(fsm != NULL);
	assert(cb != NULL);

	ei = fsm->endid_info;
	if (ei == NULL) {
		return;
	}

	assert(state != BUCKET_NO_STATE);

	hash = hash_state(state);
	bucket_count = ei->bucket_count;
	mask = bucket_count - 1;
	assert((mask & bucket_count) == 0); /* power of 2 */

#if LOG_ENDIDS > 2
	fprintf(stderr, "fsm_endid_iter_state: state %d -> hash %x\n",
	    state, hash);
#endif

#if LOG_ENDIDS > 3
	dump_buckets("fsm_endid_iter_state, before", ei);
#endif

	for (b_i = 0; b_i < bucket_count; b_i++) {
		struct endid_info_bucket *b = &ei->buckets[(hash + b_i) & mask];
#if LOG_ENDIDS > 2
		fprintf(stderr, "fsm_endid_iter_state: bucket [%ld/%ld]: %d\n",
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
