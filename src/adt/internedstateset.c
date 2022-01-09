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

#include <fsm/fsm.h>

#include <adt/alloc.h>
#include <adt/stateset.h>
#include <adt/internedstateset.h>

#include "common/check.h"

/* Adapted from _Confluently Persistent Sets and Maps_ by Olle Liljenzin.
 *
 * This is a confluent persistent set pool, for interning sets of state
 * IDs. If multiple sets are constructed with the same elements, they
 * will end up with references to the same root node no matter what
 * order the IDs are added, which allows constant-time testing for
 * equality. Partially overlapping sets will be built using shared
 * subtrees when possible, which can save quite a lot of memory.
 *
 * A hash table is used to intern individual nodes, and reference
 * counting tracks when nodes can be returned to the freelist and
 * reused. There can be a lot of short-lived nodes constructed during
 * subtree rebalancing. Reference counting keeps the working set small,
 * but generational tracing collection may also be worth exploring at
 * some point. */

/* These must both be a power of 2 and > 2. */
#define ISS_POOL_DEF_CEIL 32
#define CACHE_BUCKETS_DEF_COUNT 32

#define LOG_INTERNEDSTATESET (0+0)
#define LOG_REFERENCE_COUNTING 0

/* This should be 0 in production, 1 or 2 for hash table debugging. */
#define LOG_HTAB 0
#define EXCESS_MAX_PROBES 32	/* abort when LOG_HTAB > 0 */

#define EXPENSIVE_INTEGRITY_CHECK 0

#if LOG_INTERNEDSTATESET || LOG_HTAB
#include <stdlib.h>
#include <stdio.h>
#endif

#if LOG_REFERENCE_COUNTING
#define LOG_REF(DESCR, NODE_ID, INC)					\
	do {								\
		const iss_node_id id = NODE_ID;				\
		if (id != NO_ID) {					\
			fprintf(stderr, "REF %s -- %s: node[%d]: %s\n",	\
			    __func__, DESCR, NODE_ID, INC);		\
		}							\
	} while(0)
#else
#define LOG_REF(DESCR, NODE_ID, INC)
#endif

/* Just an abbreviation. */
typedef interned_state_set_id iss_node_id;

/* Also used as an ID for the empty set. */
#define NO_ID ((iss_node_id)-1)

/* Placeholder for deleted nodes in the node cache. */
#define TOMBSTONE_ID ((iss_node_id)-2)

/* If the refcount gets this high, rather than rolling over, the node
 * just sticks around until the whole pool is freed. */
#define STUCK_REFCOUNT (UINT32_MAX)

struct interned_state_set_pool {
	const struct fsm_alloc *alloc;

	/* The ISS tree nodes are stored in a dynamic array, along with a freelist. */
	size_t pool_ceil;
	iss_node_id pool_freelist_head; /* can be NO_ID if the freelist is empty */
	struct iss_node {
		/* If .refcount is 0, this node is on the freelist and .r is the
		 * ID of the next node, or NO_ID for the end of the freelist.
		 * Otherwise, the refcount is 1 <= refcount <= STUCK_REFCOUNT. */
		uint32_t refcount;
		iss_node_id id;

		/* Branch or leaf node's state_id. */
		fsm_state_t state;

		/* Cache of subtree hash, used to find existing nodes whose
		 * subtrees contain the same state IDs. */
		uint64_t hash;

		/* This can be NULL. The actual state_set is only instantiated
		 * when requested with interned_state_set_retain. */
		struct state_set *set;

		/* IDs for left and right children, can be NO_ID for leaf nodes.
		 * When on the freelist, .l should be the node's ID (which can
		 * never happen in the well-formed tree) and .r is the next
		 * node on the freelist. */
		iss_node_id l;
		iss_node_id r;
	} *pool;

	/* Hash table for finding existing iss_nodes.
	 *
	 * This tracks the number of live entries, but also tombstones
	 * marking entries that have been removed. These need to be
	 * distinct from empty buckets (NO_ID) so that hash collisions
	 * placed in buckets after them are not lost when deleting
	 * nodes. They are also handled differently when the hash table
	 * becomes too full -- if it's mostly filled with tombstones,
	 * it may not need to grow, just remove the tombstones and rehash
	 * the other entries. */
	struct issp_cache {
		uint32_t count;	/* bucket count */
		uint32_t used_entries;
		uint32_t used_tombstones;
		struct cache_bucket {
			iss_node_id id;			/* NO_ID if empty */
		} *buckets;
	} cache;
};

/* Magic values used to make certain things stand out in a debugger. */
#define NODE_STATE_ON_NEW_FREELIST    77777777
#define NODE_STATE_ON_GROWN_FREELIST  88888888
#define NODE_STATE_ON_REUSED_FREELIST 99999999

#define NO_DOOMED_ID ((iss_node_id)-2) /* just print */
static void
integrity_check(struct interned_state_set_pool *p, iss_node_id doomed_id);

static bool
is_empty_id(iss_node_id id)
{
	return id == NO_ID;
}

static struct iss_node *
get_iss_node(const struct interned_state_set_pool *p, iss_node_id node_id)
{
#if LOG_INTERNEDSTATESET > 4
	fprintf(stderr, "%s: node_id %d\n", __func__, node_id);
#endif
	if (node_id == NO_ID) {
		return NULL;
	}

	assert(node_id < p->pool_ceil);
	return &p->pool[node_id];
}

/* This will be in include/adt/hash.h later. */

/* 32 and 64-bit approximations of the golden ratio. */
#define FSM_PHI_32 0x9e3779b9UL
#define FSM_PHI_64 0x9e3779b97f4a7c15UL

SUPPRESS_EXPECTED_UNSIGNED_INTEGER_OVERFLOW()
static __inline__ uint64_t
fsm_hash_id(unsigned id)
{
	return FSM_PHI_64 * (id + 1);
}

SUPPRESS_EXPECTED_UNSIGNED_INTEGER_OVERFLOW()
static uint64_t
hash_iss_node(const struct interned_state_set_pool *p, const struct iss_node *n, bool transitive)
{
	assert(p != NULL);
	assert(n != NULL);

	uint64_t res;

	/* This hash is derived from the state IDs, rather than the node
	 * IDs, which will change from one run to another. */
	res = fsm_hash_id(n->state);

	/* These should be used to quickly locate existing nodes within
	 * the cache, but the subtree hashes should not be included when
	 * deciding whether to rebalance the tree inside iss_union
	 * because that can prevent the tree from converging on the same
	 * overall shape when keys are added in a different order. */
	if (transitive) {
		const struct iss_node *nl = get_iss_node(p, n->l);
		const struct iss_node *nr = get_iss_node(p, n->r);

		/* Two different primes close to 2**32 */
		if (nl != NULL) {
			const uint64_t lprime = ((uint64_t)1 << 32) - 267;
			res += lprime * nl->hash;
		}
		if (nr != NULL) {
			const uint64_t rprime = ((uint64_t)1 << 32) - 209;
			res += rprime * nr->hash;
		}

#if LOG_INTERNEDSTATESET > 3
		fprintf(stderr, "%s: res 0x%016lx <=== state %d, l: %d (0x%016lx), r: %d (0x%016lx)\n",
		    __func__, res, n->state,
		    n->l, n->l == NO_ID ? 0 : nl->hash,
		    n->r, n->r == NO_ID ? 0 : nr->hash);
#endif
	}

	return res;
}

/* Get a node from the freelist, growing it if necessary.
 * Returns true and sets *node_id, or returns false on alloc error. */
static bool
freelist_pop(struct interned_state_set_pool *p,
    iss_node_id *node_id)
{
	if (p->pool_freelist_head == NO_ID) {
		/* TODO: Instead of always doubling, this could double up to
		 * a certain size and then add a fixed amount. */
		const size_t nceil = 2*p->pool_ceil;

#if LOG_INTERNEDSTATESET > 1
	fprintf(stderr, "%s: %zu -> %zu\n", __func__, nceil, p->pool_ceil);
#endif

		assert(nceil > p->pool_ceil);
		struct iss_node *npool = f_realloc(p->alloc,
		    p->pool, nceil * sizeof(npool[0]));
		if (npool == NULL) {
			return false;
		}

		/* update freelist */
		for (size_t i = p->pool_ceil; i < nceil; i++) {
			npool[i].refcount = 0;
			npool[i].state = NODE_STATE_ON_GROWN_FREELIST;
			npool[i].set = NULL;
			npool[i].l = i;
			npool[i].r = (i == nceil - 1 ? NO_ID : i + 1);
		}
		p->pool_freelist_head = p->pool_ceil;

		p->pool = npool;
		p->pool_ceil = nceil;
	}
	const iss_node_id head = p->pool_freelist_head;
#if LOG_INTERNEDSTATESET > 3
	fprintf(stderr, "%s: current freelist head %u\n", __func__, head);
#endif

	struct iss_node *n = get_iss_node(p, head);

	assert(n->refcount == 0);
	assert(n->l == head);
	assert(n->set == NULL);
	n->l = NO_ID;

	p->pool_freelist_head = n->r;
#if LOG_INTERNEDSTATESET > 3
	fprintf(stderr, "%s: new freelist head %u\n", __func__, n->r);
#endif

	*node_id = head;
	return true;
}

static void
freelist_push(struct interned_state_set_pool *p, iss_node_id node_id)
{
#if LOG_INTERNEDSTATESET > 1
	fprintf(stderr, "%s: node_id %u\n", __func__, node_id);
#endif
	struct iss_node *n = get_iss_node(p, node_id);
	assert(n->refcount == 0);
	n->l = node_id;
	n->r = p->pool_freelist_head;
	n->state = NODE_STATE_ON_REUSED_FREELIST;
	p->pool_freelist_head = node_id;
}

struct interned_state_set_pool *
interned_state_set_pool_alloc(const struct fsm_alloc *a)
{
	struct interned_state_set_pool *res = NULL;
	struct iss_node *pool = NULL;
	struct cache_bucket *buckets = NULL;

	size_t i;
	res = f_calloc(a, 1, sizeof(*res));
	if (res == NULL) {
		goto cleanup;
	}

	/* allocate pool */
	pool = f_malloc(a, ISS_POOL_DEF_CEIL * sizeof(*pool));
	if (pool == NULL) {
		goto cleanup;
	}

	/* allocate buckets */
	buckets = f_malloc(a, CACHE_BUCKETS_DEF_COUNT * sizeof(*buckets));
	if (buckets == NULL) {
		goto cleanup;
	}

	/* init buckets */
	for (i = 0; i < CACHE_BUCKETS_DEF_COUNT; i++) {
		buckets[i].id = NO_ID;
	}

	/* init freelist */
	for (i = 0; i < ISS_POOL_DEF_CEIL; i++) {
		pool[i].refcount = 0;
		pool[i].set = NULL;
		pool[i].state = NODE_STATE_ON_NEW_FREELIST;
		pool[i].l = i;
		pool[i].r = (i == ISS_POOL_DEF_CEIL - 1 ? NO_ID : i + 1);
	}

	res->alloc = a;
	res->pool_ceil = ISS_POOL_DEF_CEIL;
	res->pool = pool;
	res->cache.count = CACHE_BUCKETS_DEF_COUNT;
	res->cache.buckets = buckets;
	res->pool_freelist_head = 0;

#if LOG_INTERNEDSTATESET > 1
	fprintf(stderr, "%s: allocated with pool of %zu, cache with %u buckets\n",
	    __func__, res->pool_ceil, res->cache.count);
#endif
	return res;

cleanup:
	if (res != NULL) {
		f_free(a, res);
	}
	if (pool != NULL) {
		f_free(a, pool);
	}
	if (buckets != NULL) {
		f_free(a, buckets);
	}
	return NULL;
}

void
interned_state_set_pool_free(struct interned_state_set_pool *pool)
{
	if (pool == NULL) {
		return;
	}

#if LOG_INTERNEDSTATESET > 1
	fprintf(stderr, "%s: buckets: (e: %u, t: %u, e+t: %u)/%u used (%g%%)\n",
	    __func__, pool->cache.used_entries, pool->cache.used_tombstones,
	    pool->cache.used_entries + pool->cache.used_tombstones, pool->cache.count,
	    (100.0 * (pool->cache.used_entries + pool->cache.used_tombstones))/pool->cache.count);
	interned_state_set_dump(pool, NULL);
#endif

	for (size_t i = 0; i < pool->pool_ceil; i++) {
		if (pool->pool[i].set != NULL) {
			state_set_free(pool->pool[i].set);
		}
	}

	f_free(pool->alloc, pool->pool);
	f_free(pool->alloc, pool->cache.buckets);
	f_free(pool->alloc, pool);
}

interned_state_set_id
interned_state_set_empty(struct interned_state_set_pool *pool)
{
	(void)pool;
	return NO_ID;
}

#if LOG_HTAB
static size_t max_probes = 0;

static void
dump_max_probes(void)
{
	fprintf(stderr, "max_probes: %zu\n", max_probes);
}
#endif

static void
dump_htab(struct interned_state_set_pool *pool)
{
#if LOG_HTAB
	uint8_t bits = 0;
	size_t used = 0;
	size_t tombstones = 0;
	const struct issp_cache *c = &pool->cache;
	fprintf(stderr, "==== %s: buckets: (e:%u t:%u, e+t:%u)/%u used (%g%%)\n",
	    __func__, c->used_entries, c->used_tombstones,
	    c->used_entries + c->used_tombstones, c->count,
	    (100.0 * (c->used_entries + c->used_tombstones))/c->count);

	for (size_t i = 0; i < c->count; i++) {
		const iss_node_id id = c->buckets[i].id;
		if (id != NO_ID) {
			if (id == TOMBSTONE_ID) {
				tombstones++;
				bits++;
			} else {
				used++;
				bits++;
			}
		}

		if ((i & 7) == 7) {
			fprintf(stderr, "%c",
			    bits == 0 ? '.' : bits + '0');
			bits = 0;
		}
		if ((i & 511) == 511) {
			fprintf(stderr, "\n");
		}
	}
	if ((c->count & 511) != 511) {
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "==== tombstones: %zu, used %zu, t+u %zu/%u (%g%%)\n",
	    tombstones, used, tombstones + used, c->count,
	    (100.0 * (tombstones + used))/c->count);
#else
	(void)pool;
#endif
}

SUPPRESS_EXPECTED_UNSIGNED_INTEGER_OVERFLOW()
static bool
grow_or_clean_hash_table(struct interned_state_set_pool *pool)
{
	const uint32_t ocount = pool->cache.count;

	/* If the table is < 1/4 full when the tombstones aren't counted,
	 * then just copy the entries to a new htab of the same size,
	 * discarding all the tombstones. It's not worth growing the hash
	 * table if it would be nearly empty without tombstones. */
	const uint32_t ncount = pool->cache.used_entries < ocount/4
	    ? ocount : 2*ocount;

	assert(ncount >= pool->cache.count);
	struct cache_bucket *obuckets = pool->cache.buckets;
	struct cache_bucket *nbuckets = f_malloc(pool->alloc,
	    ncount * sizeof(nbuckets[0]));
	size_t nused_entries = 0;
	size_t old_tombstones = 0;

	if (nbuckets == NULL) {
		return false;
	}

#if LOG_HTAB
	fprintf(stderr, "#### BEFORE grow_or_clean_hash_table\n");
	dump_htab(pool);
#endif

#if LOG_INTERNEDSTATESET > 1
	fprintf(stderr, "%s: %u -> %u\n", __func__, ocount, ncount);
#endif

	size_t i;
	for (i = 0; i < ncount; i++) {
		nbuckets[i].id = NO_ID;
	}

	const uint64_t nmask = ncount - 1;
	for (size_t o_i = 0; o_i < ocount; o_i++) {
		iss_node_id id = obuckets[o_i].id;
		if (id == TOMBSTONE_ID) {
			old_tombstones++;
			continue;
		} else if (id == NO_ID) {
			continue;
		}

		const struct iss_node *n = get_iss_node(pool, id);
		for (uint64_t n_i = 0; n_i < (uint64_t)ncount; n_i++) {
			const uint64_t b = (n->hash + n_i) & nmask;
			if (nbuckets[b].id == NO_ID) {
				nbuckets[b].id = id;
				nused_entries++;
				break;
			}
		}
	}

	f_free(pool->alloc, obuckets);
	pool->cache.count = ncount;
	pool->cache.buckets = nbuckets;
	pool->cache.used_tombstones = 0;
	pool->cache.used_entries = nused_entries;

#if LOG_HTAB
	fprintf(stderr, "#### AFTER grow_or_clean_hash_table\n");
	dump_htab(pool);

	fprintf(stderr, "nused_entries %zu, old_tombstones %zu\n",
	    nused_entries, old_tombstones);
#endif

	return true;
}

static void
retain_node_id(struct interned_state_set_pool *p, iss_node_id id)
{
	struct iss_node *n = get_iss_node(p, id);
	if (n == NULL) {
		return;
	}
	if (n->refcount != STUCK_REFCOUNT) {
#if LOG_INTERNEDSTATESET > 3
		fprintf(stderr, "%s: node[%d], refcount %d -> %d\n",
		    __func__, id, n->refcount, n->refcount + 1);
#endif
		n->refcount++;
	}
}

static void
release_node_id(struct interned_state_set_pool *p, iss_node_id *p_id)
{
	bool freed = false;
	assert(p_id != NULL);
	const iss_node_id id = *p_id;
	if (id == NO_ID) {
		return;
	}
	*p_id = NO_ID;		/* consume caller's reference */

	struct iss_node *n = get_iss_node(p, id);
	assert(n != NULL);
	if (n->refcount != STUCK_REFCOUNT) {
#if LOG_INTERNEDSTATESET > 3
		fprintf(stderr, "%s: node[%d], refcount %d -> %d\n",
		    __func__, id, n->refcount,
		    n->refcount - (n->refcount == STUCK_REFCOUNT ? 0 : 1));
#endif
		assert(n->refcount > 0);

		n->refcount--;
		if (n->refcount == 0) {
			if (n->set != NULL) {
				state_set_free(n->set);
				n->set = NULL;
			}

			release_node_id(p, &n->l);
			release_node_id(p, &n->r);

			/* Find the released node in the hash table and replace
			 * it with a tombstone, so it will be skipped over while
			 * looking for collisions. The tombstone may be refilled
			 * by later additions, and all tombstones are discarded
			 * when growing the hash table. */
			const uint64_t mask = p->cache.count - 1;
			for (uint64_t b_i = 0; b_i < p->cache.count; b_i++) {
				const uint64_t b = (n->hash + b_i) & mask;
				if (p->cache.buckets[b].id == n->id) {
					p->cache.buckets[b].id = TOMBSTONE_ID;

					assert(p->cache.used_entries > 0);
					p->cache.used_entries--;
					p->cache.used_tombstones++;

#if LOG_HTAB > 1
					fprintf(stderr, "%s %d: htab delete, used %d -> %d, id %d\n",
					    __func__, __LINE__,
					    p->cache.used_entries + 1, p->cache.used_entries, n->id);
#endif
					break;
				}
			}

			freed = true;
			freelist_push(p, id);
		}
	}

	if (freed) {
		/* check for dead references */
		integrity_check(p, id);
	}
}

/* Create a node with a given (state, left_id, right_id) tuple,
 * or reuse an existing one if available.
 *
 * This transfers ownership of r_id and l_id from the caller, so it
 * takes pointers and writes NO_ID into them. */
static bool
intern_treap_node(struct interned_state_set_pool *pool,
    fsm_state_t state, iss_node_id *p_l_id, iss_node_id *p_r_id,
	iss_node_id *res_id)
{
	/* Transfer ownership. */
	assert(p_l_id != NULL);
	iss_node_id l_id = *p_l_id;
	*p_l_id = NO_ID;

	assert(p_r_id != NULL);
	iss_node_id r_id = *p_r_id;
	*p_r_id = NO_ID;

	/* refcount: If iss is saved as a new node, l_id and r_id's
	 * ownership is transferred to it, otherwise they will be
	 * released later. */
	struct iss_node iss = {
		.id = NO_ID,	/* to be assigned */
		.refcount = 1,
		.state = state,
		.l = l_id,
		.r = r_id,
	};

#if LOG_INTERNEDSTATESET > 1
	fprintf(stderr, "%s: interning state: %d, l: %d, r: %d\n",
	    __func__, state, l_id, r_id);
#endif

	const uint64_t hash = hash_iss_node(pool, &iss, true);
	iss.hash = hash;

	/* Grow or clean up hash table if excessively full. Only grow
	 * the table if it would still be too full once tombstones are
	 * removed. The table should always have at least some empty
	 * buckets or performance will degrade rapidly. */
	if (pool->cache.used_entries + pool->cache.used_tombstones/2 >= pool->cache.count/2) {
		if (!grow_or_clean_hash_table(pool)) {
			return false;
		}
	}

	/* Count how many probes were neccesary for hash table
	 * performance tuning. */
	size_t probes = 0;

	/* Check hash table for a matching node. If not found,
	 * insert in an empty bucket. There must be at least one. */
	const uint64_t mask = pool->cache.count - 1;

#define NO_FIRST_TOMBSTONE_POS ((uint64_t)-1)
	uint64_t first_tombstone_pos = NO_FIRST_TOMBSTONE_POS;

	for (uint64_t b_i = 0; b_i < pool->cache.count; b_i++) {
		const uint64_t b = (hash + b_i) & mask;
		const iss_node_id b_id = pool->cache.buckets[b].id;

#if LOG_INTERNEDSTATESET > 3
	fprintf(stderr, "%s: htab[b %lu] -> id %d\n", __func__, b, b_id);
#endif

		if (b_id == TOMBSTONE_ID) {
			if (first_tombstone_pos == NO_FIRST_TOMBSTONE_POS) {
				first_tombstone_pos = b;
			}
			continue;
		} else if (b_id == NO_ID) {
			/* not found -- reserve node, save, and return */
			iss_node_id new_id;
			if (!freelist_pop(pool, &new_id)) {
				return false;
			}
			iss.id = new_id;
			pool->pool[new_id] = iss;

			/* Optionally replace a tombstone encountered along the way,
			 * but only once we've confirmed the node we're looking for
			 * isn't already present after other collisions and tombstones. */
			const uint64_t dst = first_tombstone_pos == NO_FIRST_TOMBSTONE_POS
			    ? b : first_tombstone_pos;
			pool->cache.buckets[dst].id = new_id;

#if LOG_HTAB > 1
			fprintf(stderr, "%s %d: htab add in %s, used_entries %d -> %d, used_tombstones %d -> %d, id %d\n",
			    __func__, __LINE__,
			    (first_tombstone_pos == NO_FIRST_TOMBSTONE_POS
				? "empty" : "tombstone"),
			    pool->cache.used_entries,
			    pool->cache.used_entries + 1, pool->cache.used_tombstones,
			    pool->cache.used_tombstones - (dst == b ? 0 : 1), new_id);
#endif
			pool->cache.used_entries++;

			/* Reduce tombstone count if one was replaced. */
			if (first_tombstone_pos != NO_FIRST_TOMBSTONE_POS) {
				assert(pool->cache.used_tombstones > 0);
				pool->cache.used_tombstones--;

				/* Also clear any consecutive run of tombstones immediately
				 * before the empty bucket. They aren't doing anything
				 * besides slowing down hash table operations and
				 * possibly triggering extra hash table resizing. */
				uint64_t pos = b;
				for (;;) {
					if (pos == 0) {
						pos = pool->cache.count; /* wrap */
					}
					pos--;
					const iss_node_id pos_id = pool->cache.buckets[pos].id;
					if (pos_id != TOMBSTONE_ID) {
						break;
					}
					pool->cache.buckets[pos].id = NO_ID;
					assert(pool->cache.used_tombstones > 0);
					pool->cache.used_tombstones--;
				}
			}

			/* Now that we know we're keeping this node,
			 * keep the refcounts for the left and right
			 * children as-is, since the new node will hold
			 * a reference to them that. */
			*res_id = new_id;

#if LOG_INTERNEDSTATESET > 1
			fprintf(stderr, "%s: saving as new node[%d], in bucket %lu\n",
			    __func__, *res_id, b);
#endif
#if LOG_INTERNEDSTATESET > 4
			fprintf(stderr, "%s: hash 0x%16lx, b_i %lu, mask 0x%016lx, bucket %lu (state %d, l %d r %d)\n",
			    __func__, hash, b_i, mask, dst, iss.state, iss.l, iss.r);
#endif

#if LOG_HTAB
			if (probes > max_probes) {
				if (max_probes == 0) {
					atexit(dump_max_probes);
				}
				max_probes = probes;
				if (getenv("DUMP_HTAB")) {
					dump_htab(pool);
				} else if (LOG_HTAB > 0 && max_probes > EXCESS_MAX_PROBES) {
					/* LOG_HTAB should be 0 in production! */
					dump_htab(pool);
					fprintf(stderr, "!! EXCESS_MAX_PROBES, exiting !!\n");
					exit(EXIT_FAILURE);
				}
			}
#endif

			LOG_REF("new_node", new_id, "=1");
			return true;
		}

		const struct iss_node *b_iss = get_iss_node(pool, b_id);
#if LOG_INTERNEDSTATESET > 1
			fprintf(stderr, "%s: b_id %d => state: %d, l: %d, r: %d\n",
			    __func__, b_id, b_iss->state, b_iss->l, b_iss->r);
#endif

		/* If a matching (state, left, right) node is found, reuse it,
		 * and release the references transferred from the caller. */
		if (b_iss->state == state && b_iss->l == l_id && b_iss->r == r_id) {
			*res_id = b_iss->id;
#if LOG_INTERNEDSTATESET > 1
			fprintf(stderr, "%s: returning existing node[%d], refcount %d -> %d\n",
			    __func__, *res_id, b_iss->refcount,
			    b_iss->refcount + (b_iss->refcount == STUCK_REFCOUNT ? 0 : 1));
#endif
			/* This is creating a new reference to an
			 * existing node, so increment its refcount. */
			LOG_REF("existing_node", *res_id, "+");
			retain_node_id(pool, b_id);

			/* The left and right node references
			 * transferred in are released, because the
			 * reused node already holds a reference to
			 * them. */
			LOG_REF("new_node_reused_l", l_id, "-");
			release_node_id(pool, &l_id);
			LOG_REF("new_node_reused_r", r_id, "-");
			release_node_id(pool, &r_id);

#if LOG_HTAB
			if (probes > max_probes) {
				max_probes = probes;
				fprintf(stderr, "max_probes: %zu, hash 0x%016lx, id %d\n", max_probes, hash, state);
			}
#endif
			return true;
		} else {
			probes++;
			continue; /* collision */
		}
	}

	/* Should never get here -- the resizing/cleaning criteria for the hash
	 * table ensures it should have at least one empty node. */
	dump_htab(pool);
	assert(!"unreachable");
	return false;
}

static bool
split(struct interned_state_set_pool *pool, iss_node_id *p_n_id, fsm_state_t state,
    iss_node_id *new_l, iss_node_id *new_r)
{
	/* Transfer ownership */
	assert(p_n_id != NULL);
	iss_node_id n_id = *p_n_id;
	*p_n_id = NO_ID;

#if LOG_INTERNEDSTATESET > 2
	fprintf(stderr, "%s: n_id: %u, state: %d\n",
	    __func__, n_id, state);
#endif

	if (is_empty_id(n_id)) {
		*new_l = NO_ID;
		*new_r = NO_ID;
		return true;
	}

	struct iss_node *n = get_iss_node(pool, n_id);

	/* Don't bother splitting and recreating leaf nodes. */
	if (n->l == NO_ID && n->r == NO_ID) {
		if (state < n->state) {
			*new_l = NO_ID;
			*new_r = n_id;
		} else {
			*new_l = n_id;
			*new_r = NO_ID;
		}

#if LOG_INTERNEDSTATESET > 2
		fprintf(stderr, "%s: not splitting leaf %d, [%d, %d]\n",
		    __func__, n_id, *new_l, *new_r);
#endif
		return true;
	}

	/* Take new references to the left and right children, which
	 * will get passed along to split/intern below. */
	const fsm_state_t n_s = n->state;
	iss_node_id n_l = n->l;
	assert(n_l != n_id);
	retain_node_id(pool, n_l);

	iss_node_id n_r = n->r;
	assert(n_r != n_id);
	retain_node_id(pool, n_r);

	if (state < n_s) {
		iss_node_id l1, l2;
		if (!split(pool, &n_l, state, &l1, &l2)) {
			return false;
		}

		*new_l = l1;
		if (!intern_treap_node(pool, n_s, &l2, &n_r, new_r)) {
			return false;
		}
	} else {
		iss_node_id r1, r2;
		if (!split(pool, &n_r, state, &r1, &r2)) {
			return false;
		}

		if (!intern_treap_node(pool, n_s, &n_l, &r1, new_l)) {
			return false;
		}
		*new_r = r2;
	}

#if 1
	LOG_REF("after_split", n_id, "-");
	release_node_id(pool, &n_id);
#endif

	integrity_check(pool, NO_DOOMED_ID);
	return true;
}

/* Return the root node for a version of the old root node's ISS with
 * the state present. Existing subtrees will be reused and have their
 * refcounts updated as necessary, including the entire tree if the
 * state is already present.
 *
 * Returns true and sets *new_root on success, returns false
 * on allocation error.
 *
 * Creates new references to t1_id and t2_id, which may be released or
 * transferred below.
 *
 * This is adapted from `union` in the paper. */
static bool
iss_union(struct interned_state_set_pool *pool,
    iss_node_id *p_t1_id, iss_node_id *p_t2_id, iss_node_id *new_id)
{
	/* Transfer ownership */
	assert(p_t1_id != NULL);
	iss_node_id t1_id = *p_t1_id;
	*p_t1_id = NO_ID;

	assert(p_t2_id != NULL);
	iss_node_id t2_id = *p_t2_id;
	*p_t2_id = NO_ID;

#if LOG_INTERNEDSTATESET > 2
	fprintf(stderr, "%s: t1_id: %d, t2_id: %d\n",
	    __func__, t1_id, t2_id);
#endif

	/* NOTE: All three of these macros can trigger
	 * relocation that makes the iss_node pointers become stale. */
#define SPLIT(ID, STATE, DST_L, DST_R)		\
	if (!split(pool, ID, STATE, DST_L, DST_R)) { return false; }	\

#define UNION(N1, N2, DST)					\
	if (!iss_union(pool, N1, N2, DST)) { return false; }

#define TREAP(STATE, L, R, DST)			\
	intern_treap_node(pool, STATE, L, R, DST)

	/* Handle when one or both are NO_ID. */
	if (t1_id == t2_id) {
		/* This should only be possible in a well-formed treap when both
		 * are NO_ID. If we somehow reach this otherwise, we may need
		 * updates to the reference counting. */
		*new_id = t1_id; /* transfer reference */
		return true;
	} else if (is_empty_id(t2_id)) {
		*new_id = t1_id; /* transfer reference */
		return true;
	} else if (is_empty_id(t1_id)) {
		*new_id = t2_id; /* transfer reference */
		return true;
	}

	struct iss_node *t1 = get_iss_node(pool, t1_id);
	struct iss_node *t2 = get_iss_node(pool, t2_id);
	assert(t1 != NULL);
	assert(t2 != NULL);

	/* Save copies of these, because the operations below can cause
	 * the array of nodes to relocate. */
	iss_node_id t1_l = t1->l;
	iss_node_id t1_r = t1->r;
	iss_node_id t2_l = t2->l;
	iss_node_id t2_r = t2->r;
	const fsm_state_t t1_s = t1->state;
	const fsm_state_t t2_s = t2->state;

	/* Temporarily retain the subtrees while other
	 * tree rebalancing may be in progress. */
	LOG_REF("union_tmp_t1_l", t1_l, "+");
	LOG_REF("union_tmp_t1_r", t1_r, "+");
	LOG_REF("union_tmp_t2_l", t2_l, "+");
	LOG_REF("union_tmp_t2_r", t2_r, "+");
	retain_node_id(pool, t1_l);
	retain_node_id(pool, t1_r);
	retain_node_id(pool, t2_l);
	retain_node_id(pool, t2_r);

	if (t1_s == t2_s) {
		iss_node_id u_l, u_r;
#if LOG_INTERNEDSTATESET > 2
		fprintf(stderr, "iss_union: state == branch: ids %d, %d\n", t1_id, t2_id);
#endif
		UNION(&t1_l, &t2_l, &u_l);
		UNION(&t1_r, &t2_r, &u_r);
		if (!TREAP(t1_s, &u_l, &u_r, new_id)) {
			return false;
		}

		LOG_REF("after_union_eq_1", t1_id, "-");
		release_node_id(pool, &t1_id);
		LOG_REF("after_union_eq_2", t2_id, "-");
		release_node_id(pool, &t2_id);
		goto done;
	}

	const uint64_t h1 = hash_iss_node(pool, t1, false);
	const uint64_t h2 = hash_iss_node(pool, t2, false);

	if (h1 < h2) {
		iss_node_id l1, r1;
		iss_node_id u1, u2;
#if LOG_INTERNEDSTATESET > 2
		fprintf(stderr, "iss_union: < branch: ids %d, %d\n", t1_id, t2_id);
#endif
		SPLIT(&t1_id, t2_s, &l1, &r1);
		UNION(&l1, &t2_l, &u1);
		UNION(&r1, &t2_r, &u2);

		if (!TREAP(t2_s, &u1, &u2, new_id)) {
			return false;
		}

		LOG_REF("after_union_lt_t2", t2_id, "-");
		release_node_id(pool, &t2_id);
	} else {
		iss_node_id l2, r2;
		iss_node_id u1, u2;
#if LOG_INTERNEDSTATESET > 2
		fprintf(stderr, "iss_union: >= branch: ids %d, %d\n", t1_id, t2_id);
#endif
		SPLIT(&t2_id, t1_s, &l2, &r2);
		UNION(&t1_l, &l2, &u1);
		UNION(&t1_r, &r2, &u2);

		if (!TREAP(t1_s, &u1, &u2, new_id)) {
			return false;
		}

		LOG_REF("after_union_gte_t1", t1_id, "-");
		release_node_id(pool, &t1_id);
	}

	LOG_REF("union_tmp_t1_l", t1_l, "-");
	LOG_REF("union_tmp_t1_r", t1_r, "-");
	LOG_REF("union_tmp_t2_l", t2_l, "-");
	LOG_REF("union_tmp_t2_r", t2_r, "-");
	release_node_id(pool, &t1_l);
	release_node_id(pool, &t1_r);
	release_node_id(pool, &t2_l);
	release_node_id(pool, &t2_r);

#undef TREAP
#undef UNION
#undef SPLIT

done:
	return true;
}

int
interned_state_set_add(struct interned_state_set_pool *pool,
    interned_state_set_id *p_set_id, fsm_state_t state,
    interned_state_set_id *result)
{
	/* Transfer ownership */
	iss_node_id set_id = (iss_node_id)*p_set_id;
	*p_set_id = NO_ID;

	assert(set_id < pool->pool_ceil || set_id == NO_ID);

	iss_node_id new_node;
	iss_node_id none_l = NO_ID;
	iss_node_id none_r = NO_ID;
	if (!intern_treap_node(pool, state, &none_l, &none_r, &new_node)) {
		return 0;
	}

#if LOG_INTERNEDSTATESET > 3
	fprintf(stderr, "%s: interned node[%u] with [%d]\n",
	    __func__, new_node, state);
#endif

	/* Get a node from the freelist. Insert into the
	 * appropriate side, rebalance on the way up, and return
	 * the new head, reusing existing nodes (and incrementing refcounts)
	 * whenever possible. */
	iss_node_id new_root_id;
	if (iss_union(pool, &set_id, &new_node, &new_root_id)) {
#if LOG_INTERNEDSTATESET > 3
		fprintf(stderr, "%s: new_root_id %u\n", __func__, new_root_id);
#endif
		integrity_check(pool, NO_DOOMED_ID);
		*result = new_root_id;
		return 1;
	} else {
		return 0;
	}
}

static bool
construct_state_set(struct interned_state_set_pool *p,
    struct state_set **dst, struct iss_node *n)
{
	struct iss_node *nl = get_iss_node(p, n->l);
	struct iss_node *nr = get_iss_node(p, n->r);
	if (nl != NULL) {
		if (!construct_state_set(p, dst, nl)) {
			return false;
		}
	}

	if (!state_set_add(dst, p->alloc, n->state)) {
		return false;
	}

	if (nr != NULL) {
		if (!construct_state_set(p, dst, nr)) {
			return false;
		}
	}

	return true;
}

struct state_set *
interned_state_set_retain(struct interned_state_set_pool *p, interned_state_set_id set_id)
{
	struct iss_node *n = get_iss_node(p, set_id);
	assert(n != NULL);

	retain_node_id(p, set_id);

	if (n->set != NULL) {
		return n->set;
	}

	assert(n->set == NULL);
	if (!construct_state_set(p, &n->set, n)) {
		return NULL;
	}
	return n->set;
}

static void
integrity_check(struct interned_state_set_pool *p, iss_node_id doomed_id)
{
	if (!EXPENSIVE_INTEGRITY_CHECK) { return; }

	bool ok = true;
#if LOG_INTERNEDSTATESET > 1
	if (doomed_id != NO_DOOMED_ID) {
		fprintf(stderr, "%s: scanning for %d\n", __func__, doomed_id);
	}
#endif
	for (size_t i = 0; i < p->cache.count; i++) {
		const iss_node_id id = p->cache.buckets[i].id;
		if (id == TOMBSTONE_ID || id == NO_ID) {
			continue;
		}
		assert(id != doomed_id);
		const struct iss_node *n = get_iss_node(p, id);
		const bool fail = (n->l == doomed_id || n->r == doomed_id) && n->refcount > 0;
#if LOG_INTERNEDSTATESET
		if (LOG_INTERNEDSTATESET > 3 || fail) {
		fprintf(stderr, "%s: %zu: node[%d]: state %d, l %d, r %d, refcount %d %s\n",
		    __func__, i, id, n->state, n->l, n->r, n->refcount,
		    fail ? "\t\t<--- live reference to doomed node" : "");
		}
#endif
		if (fail) {
			ok = false;
		}
	}

	assert(ok);
}

void
interned_state_set_release(struct interned_state_set_pool *p, interned_state_set_id *set_id)
{
	release_node_id(p, set_id);
}

#if LOG_INTERNEDSTATESET > 0
static void
dump_iter(struct interned_state_set_pool *pool, struct iss_node *n, bool dot)
{
	struct iss_node *l = get_iss_node(pool, n->l);
	if (l != NULL) {
		if (dot) {
			fprintf(stderr, "\tn%d -> n%d; // L\n",
			    n->id, n->l);
		}
		dump_iter(pool, l, dot);
	}

	if (dot) {
		fprintf(stderr, "\tn%d [label=\"n%d: %d\"];\n",
		    n->id, n->id, n->state);
	} else {
		fprintf(stderr, " %d(@ %d)", n->state, n->id);
	}

	assert(n->r != n->id);
	struct iss_node *r = get_iss_node(pool, n->r);
	if (r != NULL) {
		if (dot) {
			fprintf(stderr, "\tn%d -> n%d [color=\"blue\"]; // R\n",
			    n->id, n->r);
		}
		dump_iter(pool, r, dot);
	}
}

void
interned_state_set_dump(struct interned_state_set_pool *pool, interned_state_set_id set_id)
{
#if LOG_INTERNEDSTATESET > 1
	for (size_t i = 0; i < pool->pool_ceil; i++) {
		const struct iss_node *n = &pool->pool[i];
		if (n->refcount == 0) {
#if LOG_INTERNEDSTATESET > 3
			fprintf(stderr, "%zu: freelist, next -> %d (l = %d)\n", i, n->r, n->l);
#endif
		} else {
			fprintf(stderr, "%zu: node[%d], refcount %u, state %d, l %d, r %d\n",
			    i, n->id, n->refcount, n->state, n->l, n->r);
		}
	}
	if (set == NULL) { return; }
#endif

	if (is_empty_id(set_id)) {
		fprintf(stderr, "dump[%u]: <empty>\n", set_id);
	}

	struct iss_node *n = get_iss_node(pool, set_id);
	const bool dot = getenv("DOT") != NULL;

	if (dot) {
		fprintf(stderr, "digraph {\n");
		fprintf(stderr, "\ts [style=invis];\n");
		fprintf(stderr, "\ts -> n%d\n", set_id);
	} else {
		fprintf(stderr, "dump[%u]:", set_id);
	}
	dump_iter(pool, n, dot);
	if (dot) {
		fprintf(stderr, "}\n");
	} else {
		fprintf(stderr, "\n");
	}
}
#else
void
interned_state_set_dump(struct interned_state_set_pool *pool, interned_state_set_id set_id)
{
	(void)pool;
	(void)set_id;
}
#endif
