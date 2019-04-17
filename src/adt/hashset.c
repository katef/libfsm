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
#include <adt/hashset.h>

/* XXX: explore whether we should split the bucket or not */

struct bucket {
	unsigned long hash;
	void *item;
};

struct hashset {
	const struct fsm_alloc *alloc;
	size_t nbuckets;
	size_t nitems;
	struct bucket *buckets;
	size_t maxload;
	int (*cmp)(const void *,const void *);
	unsigned long (*hash)(const void *);
	float load;
	unsigned int flags;
};

#define TOMBSTONE_HASH (~(0UL))
#define UNSET_HASH     (0UL)

static int
is_pow2(size_t n) {
	return (n & (n-1)) == 0;
}

static int
finditem(const struct hashset *s, unsigned long hash, const void *item, size_t *bp)
{
	size_t b,c,nb;

	if (s->nbuckets == 0) {
		return 0;
	}

	b = is_pow2(s->nbuckets) ? (hash & (s->nbuckets-1)) : (hash % s->nbuckets);
	nb = s->nbuckets;
	for (c=0; c < nb; c++) {
		if (s->buckets[b].hash == hash) {
			if (item == s->buckets[b].item || s->cmp(item, s->buckets[b].item) == 0) {
				*bp = b;
				return 1;
			}
		} else if (s->buckets[b].item == NULL && s->buckets[b].hash == UNSET_HASH) {
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

int
hashset_initialize(const struct fsm_alloc *a,
	struct hashset *s, size_t nb, float load,
	unsigned long (*hash)(const void *a),
	int (*cmp)(const void *a, const void *b))
{
	static const struct hashset init;
	*s = init;
	s->alloc = a;
	s->hash = hash;
	s->cmp = cmp;
	s->load = load;
	if (nb == 0) {
		return 1;
	}

	s->buckets = f_calloc(s->alloc, nb, sizeof s->buckets[0]);
	s->maxload = load * nb;
	s->nbuckets = nb;
	return (s->buckets != NULL);
}

struct hashset *
hashset_create(const struct fsm_alloc *a,
	unsigned long (*hash)(const void *a),
	int (*cmp)(const void *a, const void *b))
{
	struct hashset *s;

	s = f_malloc(a, sizeof *s);
	if (s == NULL) {
		return NULL;
	}

	hashset_initialize(a, s, 0,DEFAULT_LOAD, hash,cmp);
	return s;
}

static int
rehash(struct hashset *s)
{
	static const struct hashset hs_init;

	size_t i,nb,newsz;
	struct hashset ns;
	struct bucket *b;

	ns = hs_init;

	/* check resizing logic */
	newsz = (s->nbuckets > 0) ? 2*s->nbuckets : DEFAULT_NBUCKETS;
	ns.buckets = f_calloc(s->alloc, newsz, sizeof ns.buckets[0]);
	if (ns.buckets == NULL) {
		return 0;
	}

	ns.nbuckets = newsz;
	ns.maxload = s->load * newsz;
	ns.hash = s->hash;
	ns.cmp  = s->cmp;

	nb = s->nbuckets;
	b = s->buckets;
	for (i=0; i < nb; i++) {
		size_t bi = 0;

		if (b[i].item == NULL) {
			continue;
		}

		/* XXX: replace finditem with something that doesn't
		 * call cmp() since all items should be unique */
		finditem(&ns, b[i].hash, b[i].item, &bi);
		ns.buckets[bi] = b[i];
	}

	f_free(s->alloc, s->buckets);
	s->nbuckets = ns.nbuckets;
	s->buckets  = ns.buckets;
	s->maxload  = ns.maxload;
	return 1;
}

void *
hashset_add(struct hashset *s, void *item)
{
	size_t b=0;
	unsigned long hash = s->hash(item);

	if (s->buckets == NULL) {
		if (!rehash(s)) {
			return NULL;
		}
	}

	if (finditem(s,hash,item,&b)) {
		/* found, return item */
		return s->buckets[b].item;
	}

	/* not found, so add it */

	/* check if we need a rehash */
	if (s->nitems >= s->maxload) {
		if (!rehash(s)) {
			return NULL;
		}

		/* re-find the first available bucket */
		finditem(s,hash,item,&b);
	}

	s->buckets[b].hash = hash;
	s->buckets[b].item = item;

	s->nitems++;

	return item;
}

int
hashset_remove(struct hashset *s, const void *item)
{
	size_t b;
	unsigned long h = s->hash(item);
	b = 0;
	if (s->nitems == 0 || !finditem(s,h,item,&b)) {
		return 0;
	}

	s->buckets[b].item = NULL;
	s->buckets[b].hash = TOMBSTONE_HASH;
	s->nitems--;

	return 1;
}

void
hashset_finalize(struct hashset *s)
{
	static const struct hashset zero;

	free(s->buckets);
	*s = zero;
}

void
hashset_free(struct hashset *s)
{
	if (s != NULL) {
		hashset_finalize(s);
		free(s);
	}
}

size_t
hashset_count(const struct hashset *s)
{
	return s->nitems;
}

void
hashset_clear(struct hashset *s)
{
	s->nitems = 0;
	if (s->buckets != NULL) {
		memset(s->buckets, 0, s->nbuckets * sizeof s->buckets[0]);
	}
}

/*
 * Find if an item is in a set, and return it.
 */
void *
hashset_contains(const struct hashset *s, const void *item)
{
	unsigned long h = s->hash(item);
	size_t b = 0;
	if (finditem(s,h,item,&b)) {
		return s->buckets[b].item;
	}

	return NULL;
}

/*
 * Compare two sets for equality.
 */
int
hashset_equal(const struct hashset *a, const struct hashset *b)
{
	size_t i,n;
	struct bucket *ab;

	if (a->nitems != b->nitems) {
		return 0;
	}

	n = a->nbuckets;
	ab = a->buckets;
	for (i=0; i < n; i++) {
		if (ab[i].item != NULL && !hashset_contains(b,ab[i].item)) {
			return 0;
		}
	}

	return 1;
}

int
hashset_empty(const struct hashset *s)
{
	return s->nitems == 0;
}

static void *
hs_next(const struct hashset *s, size_t *ip)
{
	size_t i = *ip, nb = s->nbuckets;
	for (; i < nb; i++) {
		if (s->buckets[i].item != NULL) {
			*ip = i+1;
			return s->buckets[i].item;
		}
	}

	*ip = nb;
	return NULL;
}

void *
hashset_first(const struct hashset *s, struct hashset_iter *it)
{
	it->set = s;
	it->i = 0;
	return hs_next(s, &it->i);
}

void *
hashset_next(struct hashset_iter *it)
{
	return hs_next(it->set, &it->i);
}

/*
 * Return the sole item for a singleton set.
 */
void *
hashset_only(const struct hashset *s)
{
	size_t i,n;
	struct bucket *b;

	if (s->nitems == 0) {
		return NULL;
	}

	n = s->nbuckets;
	b=s->buckets;
	for (i=0; i < n; i++) {
		if (b[i].item != NULL) {
			return b[i].item;
		}
	}

	/* should not reach */
	abort();
}

static int
hs_hasnext(const struct hashset *s, size_t *ip)
{
	size_t i = *ip, nb = s->nbuckets;
	for (; i < nb; i++) {
		if (s->buckets[i].item != NULL) {
			*ip = i;
			return 1;
		}
	}

	*ip = nb;
	return 0;
}

int
hashset_hasnext(struct hashset_iter *it)
{
	return hs_hasnext(it->set, &it->i);
}

/* XXX: cheesing around uint8_t here */
extern int
siphash(const unsigned char *in, const size_t inlen, const unsigned char *k,
            unsigned char *out, const size_t outlen);

/* random key read from /dev/random */
/* XXX: replace with a seed read from /dev/random at startup... */
static const unsigned char hashk[] = {
	0x14, 0xa8, 0xff, 0x36, 0x15, 0x16, 0x2c, 0xf7, 0xf4, 0xce, 0xb8, 0x66, 0x74, 0xf4, 0x3d, 0x64,
};

unsigned long
hashptr(const void *p) {
	unsigned char v[sizeof p];
	unsigned long h;
	unsigned char ha[sizeof h];

	memcpy(&v[0], &p, sizeof p);

	siphash(v, sizeof v, hashk, &ha[0], sizeof ha);
	memcpy(&h, &ha[0], sizeof h);

	return h;
}

unsigned long
hashrec(const void *p, size_t n) {
	const unsigned char *s = p;
	unsigned long h = 0;
	unsigned char ha[sizeof h];

	siphash(s, n, hashk, &ha[0], sizeof ha);
	memcpy(&h, &ha[0], sizeof h);

	return h;
}

