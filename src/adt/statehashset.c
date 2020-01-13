/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h>

#include <adt/alloc.h>
#include <adt/hashset.h>
#include <adt/statehashset.h>

/*
 * TODO: now fsm_state_t is a numeric index, this could be a dynamically
 * allocated bitmap, instead of a hash table.
 *
 * XXX: meanwhile I'm breaking abstraction here, because item_t is not a
 * pointer type, and I've included the hashset implementation here directly
 * rather than using the generic hashset.inc
 *
 * TODO: or could re-centralise by having struct bucket provided by the
 * concrete implementations, and hashset.inc just returns the bucket.
 */

#define DEFAULT_LOAD 0.66
#define DEFAULT_NBUCKETS 4

/* XXX: explore whether we should split the bucket or not */

struct bucket {
	unsigned long hash;
	fsm_state_t item;
	unsigned int has_item:1; /* XXX: terrible for alignment */
};

struct state_hashset {
	const struct fsm_alloc *alloc;
	size_t nbuckets;
	size_t nitems;
	struct bucket *buckets;
	size_t maxload;
	int (*cmp)(const void *, const void *);
	unsigned long (*hash)(fsm_state_t);
	float load;
};

#define UNSET_HASH (0UL)

static int
state_cmpval(fsm_state_t a, fsm_state_t b)
{
	return (a > b) - (a < b);
}

static unsigned
hash_single_state(fsm_state_t state)
{
	/*
	 * We need only hash the state index here, because states are
	 * allocated uniquely. Deep comparison is effectively undertaken
	 * by the entire powerset construction algorithm, and at
	 * this point we only need a shallow comparison.
	 */
	return hashrec(&state, sizeof state);
}

static int
is_pow2(size_t n)
{
	return (n & (n-1)) == 0;
}

static int
finditem(const struct state_hashset *hashset, unsigned long hash, fsm_state_t item, size_t *bp)
{
	size_t b, c, nb;

	if (hashset->nbuckets == 0) {
		return 0;
	}

	b = is_pow2(hashset->nbuckets) ? (hash & (hashset->nbuckets-1)) : (hash % hashset->nbuckets);
	nb = hashset->nbuckets;
	for (c=0; c < nb; c++) {
		if (hashset->buckets[b].hash == hash) {
			if (item == hashset->buckets[b].item || state_cmpval(item, hashset->buckets[b].item) == 0) {
				*bp = b;
				return 1;
			}
		} else if (!hashset->buckets[b].has_item && hashset->buckets[b].hash == UNSET_HASH) {
			*bp = b;
			return 0;
		}

		if (++b == nb) {
			b = 0;
		}
	}

	*bp = nb;
	return 0;
}

struct state_hashset *
state_hashset_create(const struct fsm_alloc *a)
{
	struct state_hashset *new;

	new = f_malloc(a, sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->alloc = a;
	new->load = DEFAULT_LOAD;
	new->nbuckets = DEFAULT_NBUCKETS;
	new->maxload = new->load * new->nbuckets;
	new->nitems = 0;

	if (new->nbuckets == 0) {
		new->buckets = NULL;
	} else {
		new->buckets = f_calloc(new->alloc, new->nbuckets, sizeof new->buckets[0]);
		if (new->buckets == NULL) {
			f_free(a, new);
			return NULL;
		}
	}

	return new;
}

static void
hashset_finalize(struct state_hashset *hashset)
{
	static const struct state_hashset zero;

	f_free(hashset->alloc, hashset->buckets);
	*hashset = zero;
}

void
state_hashset_free(struct state_hashset *set)
{
	if (set == NULL) {
		return;
	}

	hashset_finalize(set);
	f_free(set->alloc, set);
}

static int
rehash(struct state_hashset *hashset)
{
	static const struct state_hashset hs_init;

	size_t i, nb, newsz;
	struct state_hashset ns;
	struct bucket *b;

	ns = hs_init;

	/* check resizing logic */
	newsz = (hashset->nbuckets > 0) ? 2 * hashset->nbuckets : DEFAULT_NBUCKETS;
	ns.buckets = f_calloc(hashset->alloc, newsz, sizeof ns.buckets[0]);
	if (ns.buckets == NULL) {
		return 0;
	}

	ns.nbuckets = newsz;
	ns.maxload = hashset->load * newsz;

	nb = hashset->nbuckets;
	b = hashset->buckets;
	for (i = 0; i < nb; i++) {
		size_t bi = 0;

		if (!b[i].has_item) {
			continue;
		}

		/* XXX: replace finditem with something that doesn't
		 * call cmp() since all items should be unique */
		finditem(&ns, b[i].hash, b[i].item, &bi);
		ns.buckets[bi] = b[i];
	}

	f_free(hashset->alloc, hashset->buckets);
	hashset->nbuckets = ns.nbuckets;
	hashset->buckets  = ns.buckets;
	hashset->maxload  = ns.maxload;
	return 1;
}

int
state_hashset_add(struct state_hashset *set, fsm_state_t item)
{
	unsigned long hash = hash_single_state(item);
	size_t b = 0;

	assert(set != NULL);

	if (set->buckets == NULL) {
		if (!rehash(set)) {
			return 0;
		}
	}

	if (finditem(set, hash, item, &b)) {
		/* found, return item */
		return set->buckets[b].item;
	}

	/* not found, so add it */

	/* check if we need a rehash */
	if (set->nitems >= set->maxload) {
		if (!rehash(set)) {
			return 0;
		}

		/* re-find the first available bucket */
		finditem(set, hash, item, &b);
	}

	set->buckets[b].hash = hash;
	set->buckets[b].item = item;
	set->buckets[b].has_item = 1;

	set->nitems++;

	return 1;
}

int
state_hashset_contains(const struct state_hashset *set, fsm_state_t item)
{
	unsigned long h = hash_single_state(item);
	size_t b = 0;

	assert(set != NULL);

	return finditem(set, h, item, &b);
}

