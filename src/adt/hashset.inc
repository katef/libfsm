/*
 * Copyright 2018-2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <adt/alloc.h>

#define DEFAULT_LOAD 0.66
#define DEFAULT_NBUCKETS 4

/* XXX: explore whether we should split the bucket or not */

struct bucket {
	unsigned long hash;
	item_t *item;
};

struct hashset {
	const struct fsm_alloc *alloc;
	size_t nbuckets;
	size_t nitems;
	struct bucket *buckets;
	size_t maxload;
	int (*cmp)(const void *, const void *);
	unsigned long (*hash)(const void *);
	float load;
};

#define TOMBSTONE_HASH (~(0UL))
#define UNSET_HASH     (0UL)

static int
is_pow2(size_t n)
{
	return (n & (n-1)) == 0;
}

static int
finditem(const struct hashset *hashset, unsigned long hash, const item_t *item, size_t *bp)
{
	size_t b, c, nb;

	if (hashset->nbuckets == 0) {
		return 0;
	}

	b = is_pow2(hashset->nbuckets) ? (hash & (hashset->nbuckets-1)) : (hash % hashset->nbuckets);
	nb = hashset->nbuckets;
	for (c=0; c < nb; c++) {
		if (hashset->buckets[b].hash == hash) {
			if (item == hashset->buckets[b].item || hashset->cmp(&item, &hashset->buckets[b].item) == 0) {
				*bp = b;
				return 1;
			}
		} else if (hashset->buckets[b].item == NULL && hashset->buckets[b].hash == UNSET_HASH) {
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

static struct hashset *
hashset_create(const struct fsm_alloc *a,
	unsigned long (*hash)(const void *a),
	int (*cmp)(const void *a, const void *b))
{
	struct hashset *new;

	new = f_malloc(a, sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->alloc = a;
	new->hash = hash;
	new->cmp = cmp;
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

static int
rehash(struct hashset *hashset)
{
	static const struct hashset hs_init;

	size_t i, nb, newsz;
	struct hashset ns;
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
	ns.hash = hashset->hash;
	ns.cmp  = hashset->cmp;

	nb = hashset->nbuckets;
	b = hashset->buckets;
	for (i = 0; i < nb; i++) {
		size_t bi = 0;

		if (b[i].item == NULL) {
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

static item_t *
hashset_add(struct hashset *hashset, item_t *item)
{
	unsigned long hash = hashset->hash(item);
	size_t b = 0;

	if (hashset->buckets == NULL) {
		if (!rehash(hashset)) {
			return NULL;
		}
	}

	if (finditem(hashset, hash, item, &b)) {
		/* found, return item */
		return hashset->buckets[b].item;
	}

	/* not found, so add it */

	/* check if we need a rehash */
	if (hashset->nitems >= hashset->maxload) {
		if (!rehash(hashset)) {
			return NULL;
		}

		/* re-find the first available bucket */
		finditem(hashset, hash, item, &b);
	}

	hashset->buckets[b].hash = hash;
	hashset->buckets[b].item = item;

	hashset->nitems++;

	return item;
}

static int
hashset_remove(struct hashset *hashset, const item_t *item)
{
	size_t b;
	unsigned long h = hashset->hash(item);
	b = 0;
	if (hashset->nitems == 0 || !finditem(hashset, h, item, &b)) {
		return 0;
	}

	hashset->buckets[b].item = NULL;
	hashset->buckets[b].hash = TOMBSTONE_HASH;
	hashset->nitems--;

	return 1;
}

static void
hashset_finalize(struct hashset *hashset)
{
	static const struct hashset zero;

	free(hashset->buckets);
	*hashset = zero;
}

static void
hashset_free(struct hashset *hashset)
{
	if (hashset == NULL) {
		return;
	}

	hashset_finalize(hashset);
	free(hashset);
}

static size_t
hashset_count(const struct hashset *hashset)
{
	return hashset->nitems;
}

static void
hashset_clear(struct hashset *hashset)
{
	hashset->nitems = 0;
	if (hashset->buckets != NULL) {
		memset(hashset->buckets, 0, hashset->nbuckets * sizeof hashset->buckets[0]);
	}
}

/*
 * Find if an item is in a hashset, and return it.
 */
static item_t *
hashset_find(const struct hashset *hashset, const item_t *item)
{
	unsigned long h = hashset->hash(item);
	size_t b = 0;
	if (finditem(hashset, h, item, &b)) {
		return hashset->buckets[b].item;
	}

	return NULL;
}

/*
 * Compare two sets for equality.
 */
static int
hashset_equal(const struct hashset *a, const struct hashset *b)
{
	size_t i, n;
	struct bucket *ab;

	if (a->nitems != b->nitems) {
		return 0;
	}

	n = a->nbuckets;
	ab = a->buckets;
	for (i = 0; i < n; i++) {
		if (ab[i].item != NULL && !hashset_find(b, ab[i].item)) {
			return 0;
		}
	}

	return 1;
}

static int
hashset_empty(const struct hashset *hashset)
{
	return hashset->nitems == 0;
}

static item_t *
hs_next(const struct hashset *hashset, size_t *ip)
{
	size_t i = *ip, nb = hashset->nbuckets;
	for (; i < nb; i++) {
		if (hashset->buckets[i].item != NULL) {
			*ip = i+1;
			return hashset->buckets[i].item;
		}
	}

	*ip = nb;
	return NULL;
}

static item_t *
hashset_first(const struct hashset *hashset, struct hashset_iter *it)
{
	it->hashset = hashset;
	it->i = 0;
	return hs_next(hashset, &it->i);
}

static item_t *
hashset_next(struct hashset_iter *it)
{
	return hs_next(it->hashset, &it->i);
}

/*
 * Return the sole item for a singleton set.
 */
static item_t *
hashset_only(const struct hashset *hashset)
{
	size_t i, n;
	struct bucket *b;

	if (hashset->nitems == 0) {
		return NULL;
	}

	n = hashset->nbuckets;
	b = hashset->buckets;
	for (i = 0; i < n; i++) {
		if (b[i].item != NULL) {
			return b[i].item;
		}
	}

	/* should not reach */
	abort();
}

static int
hs_hasnext(const struct hashset *hashset, size_t *ip)
{
	size_t i = *ip, nb = hashset->nbuckets;
	for (; i < nb; i++) {
		if (hashset->buckets[i].item != NULL) {
			*ip = i;
			return 1;
		}
	}

	*ip = nb;
	return 0;
}

static int
hashset_hasnext(struct hashset_iter *it)
{
	return hs_hasnext(it->hashset, &it->i);
}

