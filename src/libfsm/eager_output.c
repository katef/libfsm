/*
 * Copyright 2024 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <stdio.h>
#include <assert.h>

#include "internal.h"

#include <fsm/pred.h>
#include <fsm/print.h>

#include <adt/edgeset.h>
#include <adt/hash.h>
#include <adt/stateset.h>

#include "eager_output.h"

#define LOG_LEVEL 0

/* must be a power of 2 */
#define DEF_BUCKET_COUNT 4
#define DEF_ENTRY_CEIL 2

struct eager_output_info {
	fsm_eager_output_cb *cb;
	void *opaque;

	struct eager_output_htab {
		size_t bucket_count;
		size_t buckets_used;
		/* empty if entry is NULL, otherwise keyed by state */
		struct eager_output_bucket {
			fsm_state_t state;
			struct eager_output_entry {
				unsigned used;
				unsigned ceil;
				fsm_end_id_t ids[];
			} *entry;
		} *buckets;
	} htab;
};

void
fsm_eager_output_set_cb(struct fsm *fsm, fsm_eager_output_cb *cb, void *opaque)
{
#if LOG_LEVEL > 2
	fprintf(stderr, "-- fsm_eager_output_set_cb %p\n", (void *)fsm);
#endif
	assert(fsm != NULL);
	assert(fsm->eager_output_info != NULL);
	fsm->eager_output_info->cb = cb;
	fsm->eager_output_info->opaque = opaque;
}

void
fsm_eager_output_get_cb(const struct fsm *fsm, fsm_eager_output_cb **cb, void **opaque)
{
	*cb = fsm->eager_output_info->cb;
	*opaque = fsm->eager_output_info->opaque;
}

int
fsm_eager_output_init(struct fsm *fsm)
{
	struct eager_output_info *ei = f_calloc(fsm->alloc, 1, sizeof(*ei));

	if (ei == NULL) { return 0; }

	struct eager_output_bucket *buckets = f_calloc(fsm->alloc,
	    DEF_BUCKET_COUNT, sizeof(buckets[0]));
	if (buckets == NULL) {
		f_free(fsm->alloc, ei);
		return 0;
	}

#if LOG_LEVEL > 2
	fprintf(stderr, "-- fsm_eager_output_init %p\n", (void *)fsm);
#endif

	ei->htab.buckets = buckets;
	ei->htab.bucket_count = DEF_BUCKET_COUNT;

	fsm->eager_output_info = ei;
	return 1;
}

void
fsm_eager_output_free(struct fsm *fsm)
{
	if (fsm == NULL || fsm->eager_output_info == NULL) { return; }

	for (size_t i = 0; i < fsm->eager_output_info->htab.bucket_count; i++) {
		struct eager_output_bucket *b = &fsm->eager_output_info->htab.buckets[i];
		if (b->entry == NULL) { continue; }
		f_free(fsm->alloc, b->entry);
	}
	f_free(fsm->alloc, fsm->eager_output_info->htab.buckets);

	f_free(fsm->alloc, fsm->eager_output_info);
#if LOG_LEVEL > 2
	fprintf(stderr, "-- fsm_eager_output_free %p\n", (void *)fsm);
#endif
	fsm->eager_output_info = NULL;
}

int
fsm_seteageroutputonends(struct fsm *fsm, fsm_output_id_t id)
{
	assert(fsm != NULL);
	const size_t count = fsm_countstates(fsm);
	for (size_t i = 0; i < count; i++) {
		if (fsm_isend(fsm, i)) {
			if (!fsm_seteageroutput(fsm, i, id)) { return 0; }
		}
	}
	return 1;
}

static bool
grow_htab(const struct fsm_alloc *alloc, struct eager_output_htab *htab)
{
	const size_t nbucket_count = 2*htab->bucket_count;
	assert(nbucket_count != 0);

	struct eager_output_bucket *nbuckets = f_calloc(alloc, nbucket_count,
	    sizeof(nbuckets[0]));
	if (nbuckets == NULL) { return false; }

	const uint64_t nmask = nbucket_count - 1;
	assert((nmask & nbucket_count) == 0); /* power of 2 */

	for (size_t ob_i = 0; ob_i < htab->bucket_count; ob_i++) {
		struct eager_output_bucket *ob = &htab->buckets[ob_i];
		if (ob->entry == NULL) { continue; }

		const uint64_t hash = hash_id(ob->state);
		for (size_t probes = 0; probes < nbucket_count; probes++) {
			const size_t nb_i = (hash + probes) & nmask;
			struct eager_output_bucket *nb = &nbuckets[nb_i];
			if (nb->entry == NULL) {
				nb->state = ob->state;
				nb->entry = ob->entry;
				break;
			} else {
				assert(nb->state != ob->state);
			}
		}
	}

	f_free(alloc, htab->buckets);
	htab->bucket_count = nbucket_count;
	htab->buckets = nbuckets;
	return true;
}

int
fsm_seteageroutput(struct fsm *fsm, fsm_state_t state, fsm_output_id_t id)
{
	assert(fsm != NULL);

	struct eager_output_info *info = fsm->eager_output_info;
	assert(info->htab.bucket_count > 0);

	if (info->htab.buckets_used >= info->htab.bucket_count/2) {
		if (!grow_htab(fsm->alloc, &info->htab)) { return 0; }
	}

	const uint64_t hash = hash_id(state);
	const uint64_t mask = info->htab.bucket_count - 1;
	assert((mask & info->htab.bucket_count) == 0); /* power of 2 */

	/* fprintf(stderr, "%s: bucket_count %zd\n", __func__, info->htab.bucket_count); */
	for (size_t probes = 0; probes < info->htab.bucket_count; probes++) {
		const size_t b_i = (hash + probes) & mask;
		struct eager_output_bucket *b = &info->htab.buckets[b_i];
		/* fprintf(stderr, "%s: state %d -> b_i %zd, state %d, entry %p\n", */
		/*     __func__, state, b_i, b->state, (void *)b->entry); */
		struct eager_output_entry *e = b->entry;
		if (e == NULL) { /* empty */
			/* add */
			const size_t alloc_sz = sizeof(*e)
			    + DEF_ENTRY_CEIL * sizeof(e->ids[0]);
			e = f_calloc(fsm->alloc, 1, alloc_sz);
			if (e == NULL) {
				return 0;
			}
			e->ceil = DEF_ENTRY_CEIL;
			b->state = state;
			b->entry = e;
			info->htab.buckets_used++;
			/* fprintf(stderr, "%s: buckets_used %zd\n", __func__, info->htab.buckets_used); */
			/* fprintf(stderr, "%s: saved new entry in bucket %zd\n", __func__, b_i); */
		} else if (b->state != state) { /* collision */
			continue;
		}

		if (e->used == e->ceil) {
			const size_t nceil = 2 * e->ceil;
			const size_t nsize = sizeof(*e)
			    + nceil * sizeof(e->ids[0]);
			struct eager_output_entry *nentry = f_realloc(fsm->alloc, e, nsize);
			if (nentry == NULL) { return 0; }
			nentry->ceil = nceil;
			b->entry = nentry;
			e = b->entry;
		}

		/* ignore duplicates */
		for (size_t i = 0; i < e->used; i++) {
			if (e->ids[i] == id) { return 1; }
		}

		e->ids[e->used++] = id;
		/* fprintf(stderr, "%s: e->ids_used %u\n", __func__, e->used); */
		fsm->states[state].has_eager_outputs = 1;
		return 1;
	}

	return 1;
}

bool
fsm_eager_output_has_eager_output(const struct fsm *fsm)
{
	assert(fsm->eager_output_info != NULL);
	const struct eager_output_htab *htab = &fsm->eager_output_info->htab;

	for (size_t b_i = 0; b_i < htab->bucket_count; b_i++) {
		struct eager_output_bucket *b = &htab->buckets[b_i];
		if (b->entry == NULL) { continue; }
		if (b->entry->used > 0) { return 1; }
	}
	return 0;
}

bool
fsm_eager_output_state_has_eager_output(const struct fsm *fsm, fsm_state_t state)
{
	assert(state < fsm->statecount);
	return fsm->states[state].has_eager_outputs;
}

void
fsm_eager_output_iter_state(const struct fsm *fsm,
    fsm_state_t state, fsm_eager_output_iter_cb *cb, void *opaque)
{
	assert(fsm != NULL);
	assert(cb != NULL);

	const uint64_t hash = hash_id(state);

	struct eager_output_info *info = fsm->eager_output_info;
	const uint64_t mask = info->htab.bucket_count - 1;
	assert((mask & info->htab.bucket_count) == 0); /* power of 2 */

	for (size_t probes = 0; probes < info->htab.bucket_count; probes++) {
		const size_t b_i = (hash + probes) & mask;
		struct eager_output_bucket *b = &info->htab.buckets[b_i];
		/* fprintf(stderr, "%s: state %d -> b_i %zd, state %d, entry %p\n", */
		/*     __func__, state, b_i, b->state, (void *)b->entry); */
		struct eager_output_entry *e = b->entry;
		if (e == NULL) { /* empty */
			return;
		} else if (b->state != state) { /* collision */
			continue;
		}

		assert(e->used == 0 || fsm->states[state].has_eager_outputs);

		for (size_t i = 0; i < e->used; i++) {
			if (!cb(state, e->ids[i], opaque)) { return; }
		}
	}
}

static int
inc_cb(fsm_state_t state, fsm_output_id_t id, void *opaque)
{
	(void)state;
	(void)id;
	size_t *count = opaque;
	(*count)++;
	return 1;
}

/* Get the number of eager output IDs associated with a state. */
size_t
fsm_eager_output_count(const struct fsm *fsm, fsm_state_t state)
{
	size_t res = 0;
	fsm_eager_output_iter_state(fsm, state, inc_cb, (void *)&res);
	return res;
}

struct get_env {
	size_t count;
	fsm_output_id_t *buf;
};

static int
append_cb(fsm_state_t state, fsm_output_id_t id, void *opaque)
{
	struct get_env *env = opaque;
	(void)state;
	env->buf[env->count++] = id;
	return 1;
}

static int
cmp_fsm_output_id_t(const void *pa, const void *pb)
{
	const fsm_output_id_t a = *(fsm_output_id_t *)pa;
	const fsm_output_id_t b = *(fsm_output_id_t *)pb;
	return a < b ? -1 : a > b ? 1 : 0;
}

void
fsm_eager_output_get(const struct fsm *fsm, fsm_state_t state, fsm_output_id_t *buf)
{
	struct get_env env = { .buf = buf };
	fsm_eager_output_iter_state(fsm, state, append_cb, &env);
	qsort(buf, env.count, sizeof(buf[0]), cmp_fsm_output_id_t);
}

void
fsm_eager_output_iter_all(const struct fsm *fsm,
    fsm_eager_output_iter_cb *cb, void *opaque)
{
	assert(fsm != NULL);
	assert(cb != NULL);
	assert(fsm->eager_output_info != NULL);

	struct eager_output_info *info = fsm->eager_output_info;

	/* fprintf(stderr, "%s: bucket_count %zd\n", __func__, info->htab.bucket_count); */
	for (size_t b_i = 0; b_i < info->htab.bucket_count; b_i++) {
		struct eager_output_bucket *b = &info->htab.buckets[b_i];
		struct eager_output_entry *e = b->entry;
		/* fprintf(stderr, "%s: b_i %zd, state %d, entry %p\n", */
		/*     __func__, b_i, b->state, (void *)b->entry); */
		if (e == NULL) { /* empty */
			continue;
		}
		assert(e->used == 0 || fsm->states[b->state].has_eager_outputs);

		for (size_t i = 0; i < e->used; i++) {
			if (!cb(b->state, e->ids[i], opaque)) { return; }
		}
	}
}

struct dump_env {
	FILE *f;
	size_t count;
};

static int
dump_cb(fsm_state_t state, fsm_end_id_t id, void *opaque)

{
	struct dump_env *env = opaque;
	fprintf(env->f, "-- %d: id %d\n", state, id);
	env->count++;
	return 1;
}

void
fsm_eager_output_dump(FILE *f, const struct fsm *fsm)
{
	struct dump_env env = { .f = f };
	fprintf(f, "%s:\n", __func__);
	fsm_eager_output_iter_all(fsm, dump_cb, (void *)&env);
	fprintf(f, "== %zu total\n", env.count);
}

int
fsm_eager_output_compact(struct fsm *fsm, fsm_state_t *mapping, size_t mapping_count)
{
	/* Don't reallocate unless something has actually changed. */
	bool changes = false;
	for (size_t i = 0; i < mapping_count; i++) {
		if (mapping[i] != i) {
			changes = true;
			break;
		}
	}

	/* nothing to do */
	if (!changes) { return 1; }

	struct eager_output_info *eoi = fsm->eager_output_info;

	struct eager_output_bucket *nbuckets = f_calloc(fsm->alloc,
	    eoi->htab.bucket_count, sizeof(nbuckets[0]));
	if (nbuckets == NULL) {
		return 0;
	}

	const uint64_t mask = eoi->htab.bucket_count - 1;
	assert((eoi->htab.bucket_count & mask) == 0);

	for (size_t ob_i = 0; ob_i < eoi->htab.bucket_count; ob_i++) {
		const struct eager_output_bucket *ob = &eoi->htab.buckets[ob_i];
		if (ob->entry == NULL) { continue; }

		assert(ob->state < mapping_count);
		const fsm_state_t nstate = mapping[ob->state];
		if (nstate == FSM_STATE_REMAP_NO_STATE) {
			f_free(fsm->alloc, ob->entry);
			continue;
		}

		const uint64_t hash = hash_id(nstate);

		bool placed = false;
		for (size_t probes = 0; probes < eoi->htab.bucket_count; probes++) {
			const size_t nb_i = (hash + probes) & mask;
			struct eager_output_bucket *nb = &nbuckets[nb_i];
			if (nb->entry == NULL) {
				nb->state = nstate;
				nb->entry = ob->entry;
				placed = true;
				break;
			}
		}
		assert(placed);
	}

	f_free(fsm->alloc, eoi->htab.buckets);
	eoi->htab.buckets = nbuckets;
	return 1;
}
