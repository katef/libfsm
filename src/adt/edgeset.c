/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

/* If non-zero, use an experimental edge_set implementation
 * that collects edges leading to the same set in a group
 * and uses bitflags for individual labels. While there should
 * be some space savings vs. storing every <label, state> pair
 * individually, the bigger motivation is to do processing for
 * some operations (especially fsm_determinise) by group rather
 * than for every individual pair. */
#define USE_EDGE_BITSET 1

#if !USE_EDGE_BITSET
#include "libfsm/internal.h" /* XXX: for allocating struct fsm_edge, and the edges array */

#include <print/esc.h>

#include <adt/alloc.h>
#include <adt/bitmap.h>
#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>


/* This is a simple linear-probing hash table, keyed by the edge symbol.
 * Since many edge sets only contain a single item, there is a special
 * case to box the symbol and state ID boxed in the edge_set pointer.
 * Otherwise, the hash table starts with SET_INITIAL buckets and grows
 * as necessary. */
#define SET_INITIAL 8
#define SINGLETON_MAX_STATE             ((~ (uintptr_t) 0U) >> (CHAR_BIT + 1))
#define SINGLETON_ENCODE(symbol, state) ((void *) ( \
                                        	(((uintptr_t) (state)) << (CHAR_BIT + 1)) | \
                                        	(((uintptr_t) (symbol)) << 1) | \
                                        	0x1))
#define SINGLETON_DECODE_SYMBOL(ptr)    ((unsigned char) ((((uintptr_t) (ptr)) >> 1) & 0xFFU))
#define SINGLETON_DECODE_STATE(ptr)     ((fsm_state_t)   (((uintptr_t) (ptr)) >> (CHAR_BIT + 1)))
#define IS_SINGLETON(ptr)               (((uintptr_t) (ptr)) & 0x1)

/* Used to represent buckets that are not currently used;
 * tombstone is for states that were removed (and can be replaced),
 * but may be followed by other elements due to previous collisions. */
#define BUCKET_UNUSED ((fsm_state_t)-1)
#define BUCKET_TOMBSTONE ((fsm_state_t)-2)

/* 32-bit approximation of the inverse golden ratio / UINT32_MAX:
 * (sqrt(5) - 1)/2 -> 0.618, so 0.618 * 0xffffffff. See Knuth 6.4. */
#define PHI32 0x9e3779b9

#define EOI_DONE ((size_t)-1)
#define EOI_SINGLETON_SET ((size_t)-2)

struct edge_set {
	struct fsm_edge *b;	/* buckets */
	size_t count;
	size_t ceil;
};

struct edge_set *
edge_set_new(void)
{
	return NULL;
}

static struct edge_set *
edge_set_create(const struct fsm_alloc *a)
{
	struct edge_set *set;

	set = f_malloc(a, sizeof *set);
	if (set == NULL) {
		return NULL;
	}

	set->b = f_malloc(a, SET_INITIAL * sizeof *set->b);
	if (set->b == NULL) {
		f_free(a, set);
		return NULL;
	}
	memset(set->b, 0xff, SET_INITIAL * sizeof *set->b);
	assert(set->b[0].state == BUCKET_UNUSED);

	set->count = 0;
	set->ceil = SET_INITIAL;

	return set;
}

void
edge_set_free(const struct fsm_alloc *alloc, struct edge_set *set)
{
	if (set == NULL) {
		return;
	}

	if (IS_SINGLETON(set)) {
		return;
	}

	assert(set->b != NULL);

	f_free(alloc, set->b);
	f_free(alloc, set);
}

static int
grow_buckets(const struct fsm_alloc *alloc, struct edge_set *set)
{
	struct fsm_edge *nb, *ob = set->b; /* new and old buckets */
	const size_t oceil = set->ceil;
	const size_t nceil = 2 * oceil;
	const size_t nmask = nceil - 1;
	size_t o_i, n_i, added;

	/* assumed to be a power of 2 */
	assert((nceil & nmask) == 0);

	nb = f_malloc(alloc, nceil * sizeof *nb);
	if (nb == NULL) {
		return 0;
	}
	memset(nb, 0xff, nceil * sizeof *nb);
	assert(nb[0].state == BUCKET_UNUSED);

	added = 0;
	for (o_i = 0; o_i < oceil; o_i++) {
		unsigned h;
		const fsm_state_t bs = ob[o_i].state;
		if (bs == BUCKET_UNUSED || bs == BUCKET_TOMBSTONE) {
			continue;
		}

		h = PHI32 * ob[o_i].symbol;
		for (n_i = 0; n_i < nceil; n_i++) {
			const size_t b_i = (h + n_i) & nmask;
			if (nb[b_i].state != BUCKET_UNUSED) {
				assert(nb[b_i].state != BUCKET_TOMBSTONE);
				continue;
			}

			nb[b_i].symbol = ob[o_i].symbol;
			nb[b_i].state = ob[o_i].state;
			added++;
			break;
		}
	}
	assert(added == set->count);

	f_free(alloc, ob);

	set->b = nb;
	set->ceil = nceil;
	return 1;
}

int
edge_set_add(struct edge_set **setp, const struct fsm_alloc *alloc,
	unsigned char symbol, fsm_state_t state)
{
	struct edge_set *set;

	assert(setp != NULL);
/* XXX:
	assert(state <= SINGLETON_MAX_STATE);
*/

	if (IS_SINGLETON(*setp)) {
		unsigned char prev_symbol;
		fsm_state_t prev_state;

		prev_symbol = SINGLETON_DECODE_SYMBOL(*setp);
		prev_state  = SINGLETON_DECODE_STATE(*setp);

		*setp = edge_set_create(alloc);
		if (*setp == NULL) {
			return 0;
		}

		assert(!IS_SINGLETON(*setp));

		/* TODO: bulk add */
		if (!edge_set_add(setp, alloc, prev_symbol, prev_state)) {
			return 0;
		}

		if (!edge_set_add(setp, alloc, symbol, state)) {
			return 0;
		}

		return 1;
	}

	/* XXX: only if it fits */
	if (*setp == NULL) {
		*setp = SINGLETON_ENCODE(symbol, state);
		assert(SINGLETON_DECODE_SYMBOL(*setp) == symbol);
		assert(SINGLETON_DECODE_STATE(*setp) == state);
		return 1;
	}

	set = *setp;

	/* Grow buckets at 50% capacity */
	if (set->count == set->ceil/2) {
		if (!grow_buckets(alloc, set)) {
			return 0;
		}
	}

	{
		const size_t mask = set->ceil - 1;
		const unsigned h = PHI32 * symbol;

		int has_tombstone_candidate = 0;
		size_t tc_pos;

		size_t i;
		for (i = 0; i < set->ceil; i++) {
			const size_t b_i = (h + i) & mask;
			const fsm_state_t bs = set->b[b_i].state;

			/* Continue past a tombstone, but note where
			 * it was -- as long as the value being added
			 * isn't already present later, we can add it there.
			 * This fills the first tombstone, if there are more
			 * than one, because search will find it sooner. */
			if (bs == BUCKET_TOMBSTONE) {
				if (!has_tombstone_candidate) {
					has_tombstone_candidate = 1;
					tc_pos = b_i;
				}
				continue;
			} else if (bs == BUCKET_UNUSED) {
				const size_t pos = (has_tombstone_candidate
				    ? tc_pos : b_i);
				set->b[pos].state = state;
				set->b[pos].symbol = symbol;
				set->count++;

				assert(edge_set_contains(set, symbol));
				return 1;
			} else if (bs == state && set->b[b_i].symbol == symbol) {
				return 1; /* already present */
			} else {
				/* ignore other edges */
				continue;
			}
		}

		if (has_tombstone_candidate) {
			set->b[tc_pos].state = state;
			set->b[tc_pos].symbol = symbol;
			set->count++;
			return 1;
		}

		assert(!"unreachable");
		return 0;
	}
}

int
edge_set_advise_growth(struct edge_set **pset, const struct fsm_alloc *alloc,
    size_t count)
{
	/* not implemented */
	(void)pset;
	(void)alloc;
	(void)count;
	return 1;
}

int
edge_set_add_bulk(struct edge_set **pset, const struct fsm_alloc *alloc,
	uint64_t symbols[256/64], fsm_state_t state)
{
	size_t i;
	for (i = 0; i < 256; i++) {
		if (SYMBOLS_GET(symbols, i)) {
			if (!edge_set_add(pset, alloc, i, state)) {
				return 0;
			}
		}
	}
	return 1;
}

int
edge_set_add_state_set(struct edge_set **setp, const struct fsm_alloc *alloc,
	unsigned char symbol, const struct state_set *state_set)
{
	struct state_iter it;
	fsm_state_t s;

	assert(setp != NULL);

	/* TODO: bulk add */
	for (state_set_reset((void *) state_set, &it); state_set_next(&it, &s); ) {
		if (!edge_set_add(setp, alloc, symbol, s)) {
			return 0;
		}
	}

	return 1;
}

int
edge_set_find(const struct edge_set *set, unsigned char symbol,
	struct fsm_edge *e)
{
	assert(e != NULL);

	if (edge_set_empty(set)) {
		return 0;
	}

	assert(set != NULL);

	if (IS_SINGLETON(set)) {
		if (SINGLETON_DECODE_SYMBOL(set) == symbol) {
			e->symbol = symbol;
			e->state = SINGLETON_DECODE_STATE(set);
			return 1;
		}
		return 0;
	} else {
		const size_t mask = set->ceil - 1;
		const unsigned h = PHI32 * symbol;
		size_t i;
		for (i = 0; i < set->ceil; i++) {
			const size_t b_i = (h + i) & mask;
			const fsm_state_t bs = set->b[b_i].state;
			if (bs == BUCKET_UNUSED) {
				break;
			} else if (bs == BUCKET_TOMBSTONE) {
				continue; /* search past deleted */
			} else if (set->b[b_i].symbol == symbol) {
				memcpy(e, &set->b[b_i], sizeof *e);
				return 1; /* found */
			}
		}
	}

	/* not found */
	return 0;
}

int
edge_set_contains(const struct edge_set *set, unsigned char symbol)
{
	struct fsm_edge unused;
	return edge_set_find(set, symbol, &unused);
}

int
edge_set_hasnondeterminism(const struct edge_set *set, struct bm *bm)
{
	size_t i;

	assert(bm != NULL);

	if (edge_set_empty(set)) {
		return 0;
	}

	/*
	 * Instances of struct fsm_edge aren't unique, and are not ordered.
	 * The bitmap here is to identify duplicate symbols between structs.
	 *
	 * The same bitmap is shared between all states in an epsilon closure.
	 */

	if (IS_SINGLETON(set)) {
		if (bm_get(bm, SINGLETON_DECODE_SYMBOL(set))) {
			return 1;
		}

		bm_set(bm, SINGLETON_DECODE_SYMBOL(set));
		return 0;
	}

	for (i = 0; i < set->ceil; i++) {
		const fsm_state_t bs = set->b[i].state;
		if (bs == BUCKET_UNUSED || bs == BUCKET_TOMBSTONE) {
			continue; /* no element */
		}

		if (bm_get(bm, set->b[i].symbol)) {
			return 1;
		}

		bm_set(bm, set->b[i].symbol);
	}

	return 0;
}

int
edge_set_transition(const struct edge_set *set, unsigned char symbol,
	fsm_state_t *state)
{
	/*
	 * This function is meaningful for DFA only; we require a DFA
	 * by contract in order to identify a single destination state
	 * for a given symbol.
	 */
	struct fsm_edge e;
	if (!edge_set_find(set, symbol, &e)) {
		return 0;
	}
	*state = e.state;
	return 1;
}

size_t
edge_set_count(const struct edge_set *set)
{
	if (set == NULL) {
		return 0;
	}

	if (IS_SINGLETON(set)) {
		return 1;
	}

	assert(set->b != NULL);

	return set->count;
}

int
edge_set_copy(struct edge_set **dst, const struct fsm_alloc *alloc,
	const struct edge_set *src)
{
	struct edge_iter jt;
	struct fsm_edge e;

	assert(dst != NULL);

	if (edge_set_empty(src)) {
		return 1;
	}

	if (IS_SINGLETON(src)) {
		if (!edge_set_add(dst, alloc, SINGLETON_DECODE_SYMBOL(src), SINGLETON_DECODE_STATE(src))) {
			return 0;
		}

		return 1;
	}

	for (edge_set_reset(src, &jt); edge_set_next(&jt, &e); ) {
		/* TODO: bulk add */
		if (!edge_set_add(dst, alloc, e.symbol, e.state)) {
			return 0;
		}
	}

	return 1;
}

void
edge_set_remove(struct edge_set **setp, unsigned char symbol)
{
	struct edge_set *set;

	assert(setp != NULL);

	if (IS_SINGLETON(*setp)) {
		if (SINGLETON_DECODE_SYMBOL(*setp) == symbol) {
			*setp = NULL;
		}
		return;
	}

	set = *setp;

	if (edge_set_empty(set)) {
		return;
	} else {
		const size_t mask = set->ceil - 1;
		const unsigned h = PHI32 * symbol;
		size_t i;
		for (i = 0; i < set->ceil; i++) {
			const size_t b_i = (h + i) & mask;
			const fsm_state_t bs = set->b[b_i].state;
			if (bs == BUCKET_UNUSED) {
				break; /* not found */
			} else if (set->b[b_i].symbol == symbol
			    && set->b[b_i].state != BUCKET_TOMBSTONE) {
				/* Set to a distinct marker for a deleted
				 * entry; there may be entries past this
				 * due to collisions that still need to
				 * be checked. */
				set->b[b_i].state = BUCKET_TOMBSTONE;
				set->count--;
			}
		}
	}

	assert(!edge_set_contains(set, symbol));
}

void
edge_set_remove_state(struct edge_set **setp, fsm_state_t state)
{
	struct edge_set *set;
	size_t i;

	assert(setp != NULL);
	assert(state != BUCKET_UNUSED && state != BUCKET_TOMBSTONE);

	if (IS_SINGLETON(*setp)) {
		if (SINGLETON_DECODE_STATE(*setp) == state) {
			*setp = NULL;
		}
		return;
	}

	set = *setp;

	if (edge_set_empty(set)) {
		return;
	}

	/* Remove all edges with that state */
	for (i = 0; i < set->ceil; i++) {
		if (set->b[i].state == state) {
			set->b[i].state = BUCKET_TOMBSTONE;
			set->count--;
		}
	}
}

int
edge_set_compact(struct edge_set **setp, const struct fsm_alloc *alloc,
    fsm_state_remap_fun *remap, const void *opaque)
{
	struct edge_set *set;
	size_t i;

	assert(setp != NULL);

	if (IS_SINGLETON(*setp)) {
		const unsigned char symbol = SINGLETON_DECODE_SYMBOL(*setp);
		const fsm_state_t s = SINGLETON_DECODE_STATE(*setp);
		const fsm_state_t new_id = remap(s, opaque);

		if (new_id == FSM_STATE_REMAP_NO_STATE) {
			*setp = NULL;
		} else {
			*setp = SINGLETON_ENCODE(symbol, new_id);
		}
		return 1;
	}

	set = *setp;

	if (edge_set_empty(set)) {
		return 1;
	}

	i = 0;
	for (i = 0; i < set->ceil; i++) {
		const fsm_state_t to = set->b[i].state;
		fsm_state_t new_to;
		if (to == BUCKET_UNUSED || to == BUCKET_TOMBSTONE) {
			continue;
		}
		new_to = remap(to, opaque);

		if (new_to == FSM_STATE_REMAP_NO_STATE) { /* drop */
			set->b[i].state = BUCKET_TOMBSTONE;
			set->count--;
		} else {	/* keep */
			set->b[i].state = new_to;
		}
	}

	/* todo: if set->count < set->ceil/2, shrink buckets */
	return 1;
}

void
edge_set_reset(const struct edge_set *set, struct edge_iter *it)
{
	it->i = 0;
	it->set = set;
}

int
edge_set_next(struct edge_iter *it, struct fsm_edge *e)
{
	assert(it != NULL);
	assert(e != NULL);

	if (it->set == NULL) {
		return 0;
	}

	if (IS_SINGLETON(it->set)) {
		if (it->i >= 1) {
			return 0;
		}

		e->symbol = SINGLETON_DECODE_SYMBOL(it->set);
		e->state  = SINGLETON_DECODE_STATE(it->set);

		it->i++;

		return 1;
	}

	while (it->i < it->set->ceil) {
		const fsm_state_t bs = it->set->b[it->i].state;
		if (bs == BUCKET_UNUSED || bs == BUCKET_TOMBSTONE) {
			it->i++;
			continue;
		}
		*e = it->set->b[it->i];
		it->i++;
		return 1;
	}

	return 0;
}

void
edge_set_rebase(struct edge_set **setp, fsm_state_t base)
{
	struct edge_set *set;
	size_t i;

	assert(setp != NULL);

	if (IS_SINGLETON(*setp)) {
		fsm_state_t state;
		unsigned char symbol;

		state  = SINGLETON_DECODE_STATE(*setp) + base;
		symbol = SINGLETON_DECODE_SYMBOL(*setp);

		*setp = SINGLETON_ENCODE(symbol, state);
		assert(SINGLETON_DECODE_SYMBOL(*setp) == symbol);
		assert(SINGLETON_DECODE_STATE(*setp) == state);

		return;
	}

	set = *setp;

	if (edge_set_empty(set)) {
		return;
	}

	for (i = 0; i < set->ceil; i++) {
		const fsm_state_t bs = set->b[i].state;
		if (bs == BUCKET_UNUSED || bs == BUCKET_TOMBSTONE) {
			continue;
		}
		set->b[i].state += base;
	}
}

int
edge_set_replace_state(struct edge_set **setp, const struct fsm_alloc *alloc,
    fsm_state_t old, fsm_state_t new)
{
	struct edge_set *set;
	size_t i;

	assert(setp != NULL);
	assert(old != BUCKET_UNUSED);
	assert(old != BUCKET_TOMBSTONE);
	(void)alloc;

	if (IS_SINGLETON(*setp)) {
		if (SINGLETON_DECODE_STATE(*setp) == old) {
			unsigned char symbol;

			symbol = SINGLETON_DECODE_SYMBOL(*setp);

			*setp = SINGLETON_ENCODE(symbol, new);
			assert(SINGLETON_DECODE_SYMBOL(*setp) == symbol);
			assert(SINGLETON_DECODE_STATE(*setp) == new);
		}
		return 1;
	}

	set = *setp;

	if (edge_set_empty(set)) {
		return 1;
	}

	for (i = 0; i < set->ceil; i++) {
		if (set->b[i].state == old) {
			set->b[i].state = new;
		}
	}

	/* If there is now more than one edge <label, new> for
	 * any label, then the later ones need to be removed and
	 * the count adjusted. */
	{
		uint64_t seen[4];
		memset(seen, 0x00, sizeof(seen));
		for (i = 0; i < set->ceil; i++) {
			const fsm_state_t bs = set->b[i].state;
			unsigned char symbol;
			uint64_t bit;
			if (bs != new) {
				continue;
			}
			symbol = set->b[i].symbol;
			bit = (uint64_t)1 << (symbol & 63);
			if (seen[symbol/64] & bit) {
				/* remove duplicate, update count */
				set->b[i].state = BUCKET_TOMBSTONE;
				set->count--;
			} else {
				seen[symbol/64] |= bit;
			}
		}
	}

	return 1;
}

int
edge_set_empty(const struct edge_set *set)
{
	if (set == NULL) {
		return 1;
	}

	if (IS_SINGLETON(set)) {
		return 0;
	}

	return set->count == 0;
}

void
edge_set_ordered_iter_reset_to(const struct edge_set *set,
    struct edge_ordered_iter *eoi, unsigned char symbol)
{
	/* Create an ordered iterator for the hash table by figuring
	 * out which symbols are present (0x00 <= x <= 0xff, tracked
	 * in a bit set) and either yielding the next bucket for the
	 * current symbol or advancing to the next symbol present. */

	size_t i, found, mask;
	memset(eoi, 0x00, sizeof(*eoi));

	eoi->symbol = symbol;

	/* Check for special case unboxed sets first. */
	if (IS_SINGLETON(set)) {
		eoi->set = set;
		eoi->pos = EOI_SINGLETON_SET;
		return;
	} else if (edge_set_empty(set)) {
		eoi->pos = EOI_DONE;
		return;
	}

	found = 0;
	for (i = 0; i < set->ceil; i++) {
		const fsm_state_t bs = set->b[i].state;
		unsigned char symbol;
		if (bs == BUCKET_UNUSED || bs == BUCKET_TOMBSTONE) {
			continue;
		}

		symbol = set->b[i].symbol;
		eoi->symbols_used[symbol/64] |= ((uint64_t)1 << (symbol & 63));
		found++;
	}
	assert(found == set->count);
	mask = set->ceil - 1;

	/* Start out pointing to the first bucket with a matching symbol,
	 * or the first unused bucket if not present. */
	{
		const unsigned h = PHI32 * eoi->symbol;
		for (i = 0; i < set->ceil; i++) {
			const size_t b_i = (h + i) & mask;
			const fsm_state_t bs = set->b[b_i].state;
			if (bs == BUCKET_TOMBSTONE) {
				continue; /* search past deleted */
			} else if (bs == BUCKET_UNUSED) {
				eoi->pos = b_i; /* will advance to next symbol */
				break;
			} else if (set->b[b_i].symbol == eoi->symbol) {
				eoi->pos = b_i; /* pointing at first bucket */
				break;
			} else {
				continue; /* find first entry with symbol */
			}
		}
	}

	eoi->set = set;
}

static int
advance_symbol(struct edge_ordered_iter *eoi)
{
	unsigned i = eoi->symbol + 1;
	while (i < 0x100) {
		if (eoi->symbols_used[i/64] & ((uint64_t)1 << (i & 63))) {
			eoi->symbol = i;
			return 1;
		}
		i++;
	}

	eoi->pos = EOI_DONE;
	eoi->steps = 0;

	return 0;
}

void
edge_set_ordered_iter_reset(const struct edge_set *set,
    struct edge_ordered_iter *eoi)
{
	edge_set_ordered_iter_reset_to(set, eoi, 0);
}

int
edge_set_ordered_iter_next(struct edge_ordered_iter *eoi, struct fsm_edge *e)
{
	fsm_state_t bs;
	unsigned char symbol;
	const struct edge_set *set = eoi->set;
	size_t mask;

	if (eoi->pos == EOI_DONE) {
		return 0;	/* done */
	} else if (eoi->pos == EOI_SINGLETON_SET) {
		e->state = SINGLETON_DECODE_STATE(eoi->set);
		e->symbol = SINGLETON_DECODE_SYMBOL(eoi->set);
		eoi->pos = EOI_DONE;
		return 1;
	}

	mask = set->ceil - 1;

	for (;;) {
		eoi->pos &= mask;
		bs = set->b[eoi->pos].state;
		symbol = set->b[eoi->pos].symbol;
		eoi->steps++;

		if (bs == BUCKET_UNUSED || eoi->steps == set->ceil) {
			size_t i;
			unsigned h;
			/* after current symbol's entries -- check next */
			if (!advance_symbol(eoi)) {
				return 0; /* done */
			}

			h = PHI32 * eoi->symbol;
			for (i = 0; i < set->ceil; i++) {
				const size_t b_i = (h + i) & mask;
				bs = set->b[b_i].state;

				if (bs == BUCKET_TOMBSTONE) {
					continue; /* search past deleted */
				} else if (bs == BUCKET_UNUSED) {
					/* should never get here -- searching for
					 * a symbol that isn't present, but we
					 * already know what's present */
					assert(!"internal error");
				} else if (set->b[b_i].symbol != eoi->symbol) {
					continue; /* skip collision */
				} else {
					assert(set->b[b_i].symbol == eoi->symbol);

					/* yield next match and then advance */
					eoi->pos = b_i;
					eoi->steps = 0;
					memcpy(e, &set->b[eoi->pos], sizeof(*e));
					eoi->pos++;
					return 1;
				}
			}
			/* should always find a match or an unused bucket */
			assert(!"internal error");
			return 0;
		} else if (bs == BUCKET_TOMBSTONE) {
			eoi->pos++; /* skip over */
			continue;
		} else if (symbol != eoi->symbol) {
			eoi->pos++;
			continue; /* skip collision */
		} else {
			/* if pointing at next bucket, yield it */
			assert(symbol == eoi->symbol);
			memcpy(e, &set->b[eoi->pos], sizeof(*e));
			eoi->pos++;
			return 1;
		}
	}
}

#else /* USE_EDGE_BITSET */

#define LOG_BITSET 0

#include "libfsm/internal.h" /* XXX: for allocating struct fsm_edge, and the edges array */

#include <print/esc.h>

#include <adt/alloc.h>
#include <adt/bitmap.h>
#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#define DEF_EDGE_GROUP_CEIL 1

/* Array of <to, symbols> tuples, sorted by to.
 *
 * Design assumption: It is significantly more likely in practice to
 * have to be more edges with different labels going to the same state
 * than the same symbol going to different states. This does not
 * include epsilon edges, which can be stored in a state_set. */
struct edge_set {
	size_t ceil;		/* nonzero */
	size_t count;		/* <= ceil */
	struct edge_group {
		fsm_state_t to;	/* distinct */
		uint64_t symbols[256/64];
	} *groups;		/* sorted by .to */
};

#define SYMBOLS_SET(S, ID) (S[ID/64] |= (1ULL << (ID & 63)))
#define SYMBOLS_GET(S, ID) (S[ID/64] & (1ULL << (ID & 63)))
#define SYMBOLS_CLEAR(S, ID) (S[ID/64] &=~ (1ULL << (ID & 63)))

struct edge_set *
edge_set_new(void)
{
#if LOG_BITSET
	fprintf(stderr, " -- edge_set_new\n");
#endif
	return NULL;		/* -> empty set */
}

void
edge_set_free(const struct fsm_alloc *a, struct edge_set *set)
{
#if LOG_BITSET
	fprintf(stderr, " -- edge_set_free %p\n", (void *)set);
#endif
	if (set == NULL) {
		return;
	}

	f_free(a, set->groups);
	f_free(a, set);
}

static int
grow_groups(struct edge_set *set, const struct fsm_alloc *alloc)
{
	/* TODO: This could also squash out any groups where
	 * none of the symbols are set anymore. */
	const size_t nceil = 2 *set->ceil;
	struct edge_group *ng;
	assert(nceil > 0);

#if LOG_BITSET
	fprintf(stderr, " -- edge_set grow_groups: %lu -> %lu\n",
	    set->ceil, nceil);
#endif

	ng = f_realloc(alloc, set->groups,
	    nceil * sizeof(set->groups[0]));
	if (ng == NULL) {
		return 0;
	}

	set->ceil = nceil;
	set->groups = ng;

	return 1;
}

static void
dump_edge_set(const struct edge_set *set)
{
	const struct edge_group *eg;
	size_t i;
#if LOG_BITSET < 2
	return;
#endif

	if (edge_set_empty(set)) {
		fprintf(stderr, "dump_edge_set: <empty>\n");
		return;
	}

	fprintf(stderr, "dump_edge_set: %p\n", (void *)set);

	for (i = 0; i < set->count; i++) {
		eg = &set->groups[i];
		fprintf(stderr, " -- %ld: [0x%lx, 0x%lx, 0x%lx, 0x%lx] -> %u\n",
		    i,
		    eg->symbols[0], eg->symbols[1],
		    eg->symbols[2], eg->symbols[3], eg->to);
	}
}

static struct edge_set *
init_empty(const struct fsm_alloc *alloc)
{
	struct edge_set *set = f_calloc(alloc, 1, sizeof(*set));
	if (set == NULL) {
		return NULL;
	}

	set->groups = f_malloc(alloc,
	    DEF_EDGE_GROUP_CEIL * sizeof(set->groups[0]));
	if (set->groups == NULL) {
		f_free(alloc, set);
		return NULL;
	}

	set->ceil = DEF_EDGE_GROUP_CEIL;
	set->count = 0;
	return set;
}

int
edge_set_add(struct edge_set **pset, const struct fsm_alloc *alloc,
	unsigned char symbol, fsm_state_t state)
{
	uint64_t symbols[256/64] = { 0 };
	SYMBOLS_SET(symbols, symbol);
	return edge_set_add_bulk(pset, alloc, symbols, state);
}

int
edge_set_advise_growth(struct edge_set **pset, const struct fsm_alloc *alloc,
    size_t count)
{
	struct edge_set *set = *pset;
	if (set == NULL) {
		set = init_empty(alloc);
		if (set == NULL) {
			return 0;
		}
		*pset = set;
	}

	const size_t oceil = set->ceil;

	size_t nceil = 1;
	while (nceil < oceil + count) {
		nceil *= 2;
	}
	assert(nceil > 0);

#if LOG_BITSET
	fprintf(stderr, " -- edge_set advise_growth: %lu -> %lu\n",
	    set->ceil, nceil);
#endif

	struct edge_group *ng = f_realloc(alloc, set->groups,
	    nceil * sizeof(set->groups[0]));
	if (ng == NULL) {
		return 0;
	}

	set->ceil = nceil;
	set->groups = ng;

	return 1;
}

int
edge_set_add_bulk(struct edge_set **pset, const struct fsm_alloc *alloc,
	uint64_t symbols[256/64], fsm_state_t state)
{
	struct edge_set *set;
	struct edge_group *eg;
	size_t i;

	assert(pset != NULL);

	set = *pset;

	if (set == NULL) {	/* empty */
		set = init_empty(alloc);
		if (set == NULL) {
			return 0;
		}

		eg = &set->groups[0];
		eg->to = state;
		memcpy(eg->symbols, symbols, sizeof(eg->symbols));
		set->count++;

		*pset = set;

#if LOG_BITSET
		fprintf(stderr, " -- edge_set_add: symbols [0x%lx, 0x%lx, 0x%lx, 0x%lx] -> state %d on empty -> %p\n",
		    symbols[0], symbols[1], symbols[2], symbols[3],
		    state, (void *)set);
#endif
		dump_edge_set(set);

		return 1;
	}

	assert(set->ceil > 0);
	assert(set->count <= set->ceil);

#if LOG_BITSET
		fprintf(stderr, " -- edge_set_add: symbols [0x%lx, 0x%lx, 0x%lx, 0x%lx] -> state %d on %p\n",
		    symbols[0], symbols[1], symbols[2], symbols[3],
		    state, (void *)set);
#endif

	/* Linear search for a group with the same destination
	 * state, or the position where that group would go. */
	for (i = 0; i < set->count; i++) {
		eg = &set->groups[i];

		if (eg->to == state) {
			/* This API does not indicate whether that
			 * symbol -> to edge was already present. */
			size_t i;
			for (i = 0; i < 256/64; i++) {
				eg->symbols[i] |= symbols[i];
			}
			dump_edge_set(set);
			return 1;
		} else if (eg->to > state) {
			break;	/* will shift down and insert below */
		} else {
			continue;
		}
	}

	/* insert/append at i */
	if (set->count == set->ceil) {
		if (!grow_groups(set, alloc)) {
			return 0;
		}
	}

	eg = &set->groups[i];

	if (i < set->count && eg->to != state) {  /* shift down by one */
		const size_t to_mv = set->count - i;

#if LOG_BITSET
		fprintf(stderr, "   --- shifting, count %ld, i %ld, to_mv %ld\n",
		    set->count, i, to_mv);
#endif

		if (to_mv > 0) {
			memmove(&set->groups[i + 1],
			    &set->groups[i],
			    to_mv * sizeof(set->groups[i]));
		}
		eg = &set->groups[i];
	}

	eg->to = state;
	memset(eg->symbols, 0x00, sizeof(eg->symbols));
	memcpy(eg->symbols, symbols, sizeof(eg->symbols));
	set->count++;
	dump_edge_set(set);

	return 1;
}

int
edge_set_add_state_set(struct edge_set **setp, const struct fsm_alloc *alloc,
	unsigned char symbol, const struct state_set *state_set)
{
	struct state_iter si;
	fsm_state_t state;

	state_set_reset(state_set, &si);

	while (state_set_next(&si, &state)) {
		if (!edge_set_add(setp, alloc, symbol, state)) {
			return 0;
		}
	}

	return 1;
}

int
edge_set_find(const struct edge_set *set, unsigned char symbol,
	struct fsm_edge *e)
{
	const struct edge_group *eg;
	size_t i;
	if (set == NULL) {
		return 0;
	}

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_find: symbol 0x%x on %p\n",
	    symbol, (void *)set);
#endif

	for (i = 0; i < set->count; i++) {
		eg = &set->groups[i];
		if (SYMBOLS_GET(eg->symbols, symbol)) {
			e->state = eg->to;
			e->symbol = symbol;
			return 1;
		}
	}

	return 0;		/* not found */
}

int
edge_set_contains(const struct edge_set *set, unsigned char symbol)
{
	const struct edge_group *eg;
	size_t i;
	if (edge_set_empty(set)) {
		return 0;
	}

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_contains: symbol 0x%x on %p\n",
	    symbol, (void *)set);
#endif

	for (i = 0; i < set->count; i++) {
		eg = &set->groups[i];
		if (SYMBOLS_GET(eg->symbols, symbol)) {
			return 1;
		}
	}

	return 0;
}

int
edge_set_hasnondeterminism(const struct edge_set *set, struct bm *bm)
{
	size_t i, w_i;
	assert(bm != NULL);

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_hasnondeterminism: on %p\n",
	    (void *)set);
#endif

	dump_edge_set(set);

	if (edge_set_empty(set)) {
		return 0;
	}

	for (i = 0; i < set->count; i++) {
		const struct edge_group *eg = &set->groups[i];
		for (w_i = 0; w_i < 4; w_i++) {
			const uint64_t cur = eg->symbols[w_i];
			size_t b_i;
			if (cur == 0) {
				continue;
			}

#if LOG_BITSET > 1
			fprintf(stderr, " -- eshnd: [0x%lx, 0x%lx, 0x%lx, 0x%lx] + group %ld cur %ld: 0x%lx\n",
			    eg->symbols[0], eg->symbols[1],
			    eg->symbols[2], eg->symbols[3],
			    i, w_i, cur);
#endif

			for (b_i = 0; b_i < 64; b_i++) {
				if (cur & (1ULL << b_i)) {
					const size_t bit = 64*w_i + b_i;
					if (bm_get(bm, bit)) {
#if LOG_BITSET > 1
						fprintf(stderr, "-- eshnd: hit on bit %lu\n", bit);
#endif
						return 1;
					}
					bm_set(bm, bit);
				}
			}
		}
	}

	return 0;
}

int
edge_set_transition(const struct edge_set *set, unsigned char symbol,
	fsm_state_t *state)
{
	/*
	 * This function is meaningful for DFA only; we require a DFA
	 * by contract in order to identify a single destination state
	 * for a given symbol.
	 */
	struct fsm_edge e;
	if (!edge_set_find(set, symbol, &e)) {
		return 0;
	}
	*state = e.state;
	return 1;
}

size_t
edge_set_count(const struct edge_set *set)
{
	size_t res = 0;
	size_t i;

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_count: on %p\n",
	    (void *)set);
#endif

	/* This does not call edge_set_empty directly
	 * here, because that does the same scan as below,
	 * it just exits at the first label found. */
	if (set == NULL) {
		return 0;
	}

	for (i = 0; i < set->count; i++) {
		size_t w_i;
		const struct edge_group *eg = &set->groups[i];
		for (w_i = 0; w_i < 4; w_i++) {
			/* TODO: res += popcount64(w) */
			const uint64_t w = eg->symbols[w_i];
			uint64_t bit;
			if (w == 0) {
				continue;
			}
			for (bit = (uint64_t)1; bit; bit <<= 1) {
				if (w & bit) {
					res++;
				}
			}
		}
	}

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_count: %zu\n", res);
#endif
	return res;
}

static int
find_first_group_label(const struct edge_group *g, unsigned char *label)
{
	size_t i, bit;
	for (i = 0; i < 4; i++) {
		if (g->symbols[i] == 0) {
			continue;
		}
		for (bit = 0; bit < 64; bit++) {
			if (g->symbols[i] & ((uint64_t)1 << bit)) {
				*label = i*64 + bit;
				return 1;
			}
		}
	}
	return 0;
}

static int
edge_set_copy_into(struct edge_set **pdst, const struct fsm_alloc *alloc,
	const struct edge_set *src)
{
	/* Because the source and destination edge_set groups are
	 * sorted by .to, we can pairwise bulk merge them. If the
	 * to label appears in src, we can just bitwise-or the
	 * labels over in parallel; if not, we need to add it
	 * first, but it will be added at the current offset. */
	size_t sg_i, dg_i, w_i;	/* src/dst group index, word index */
	struct edge_set *dst = *pdst;

	dg_i = 0;
	sg_i = 0;
	while (sg_i < src->count) {
		const struct edge_group *src_g = &src->groups[sg_i];
		struct edge_group *dst_g = (dg_i < dst->count
		    ? &dst->groups[dg_i]
		    : NULL);
		if (dst_g == NULL || dst_g->to > src_g->to) {
			unsigned char label;

			/* If the src group is empty, skip it (that can
			 * happen as labels are added and removed,
			 * we don't currently prune empty ones),
			 * otherwise get the first label present for
			 * the edge_set_add below. */
			if (!find_first_group_label(src_g, &label)) {
				sg_i++;
				continue;
			}

			/* Insert the first label, so a group with
			 * that .to will exist at the current offset. */
			if (!edge_set_add(&dst, alloc,
				label, src_g->to)) {
				return 0;
			}

			dst_g = &dst->groups[dg_i];
		}

		assert(dst_g != NULL); /* exists now */
		if (dst_g->to < src_g->to) {
			dg_i++;
			continue;
		}

		assert(dst_g->to == src_g->to);
		for (w_i = 0; w_i < 4; w_i++) { /* bit-parallel union */
			dst_g->symbols[w_i] |= src_g->symbols[w_i];
		}
		sg_i++;
		dg_i++;
	}
	return 1;
}

int
edge_set_copy(struct edge_set **dst, const struct fsm_alloc *alloc,
	const struct edge_set *src)
{
	struct edge_set *set;
	assert(dst != NULL);

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_copy: on %p -> %p\n",
	    (void *)src, (void *)*dst);
#endif

	if (*dst != NULL) {
		/* The other `edge_set_copy` copies in the
		 * edges from src if *dst already exists. */
		return edge_set_copy_into(dst, alloc, src);
	}

	if (edge_set_empty(src)) {
		*dst = NULL;
		return 1;
	}

	set = f_calloc(alloc, 1, sizeof(*set));
	if (set == NULL) {
		return 0;
	}

	set->groups = f_malloc(alloc,
	    src->ceil * sizeof(set->groups[0]));
	if (set->groups == NULL) {
		f_free(alloc, set);
		return 0;
	}

	set->ceil = src->ceil;
	memcpy(set->groups, src->groups,
	    src->count * sizeof(src->groups[0]));
	set->count = src->count;

#if LOG_BITSET
	{
		size_t i;
		for (i = 0; i < set->count; i++) {
			fprintf(stderr, "edge_set[%zd]: to %d, [0x%lx, 0x%lx, 0x%lx, 0x%lx]\n",
			    i, set->groups[i].to,
			    set->groups[i].symbols[0],
			    set->groups[i].symbols[1],
			    set->groups[i].symbols[2],
			    set->groups[i].symbols[3]);
		}
	}
#endif

	*dst = set;
	return 1;
}

void
edge_set_remove(struct edge_set **pset, unsigned char symbol)
{
	size_t i;
	struct edge_set *set;

	assert(pset != NULL);
	set = *pset;

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_remove: symbol 0x%x on %p\n",
	    symbol, (void *)set);
#endif

	if (edge_set_empty(set)) {
		return;
	}

	/* This does not track whether edge(s) were actually removed.
	 * It also does not remove edge groups where none of the
	 * symbols are set anymore. */
	for (i = 0; i < set->count; i++) {
		struct edge_group *eg = &set->groups[i];
		SYMBOLS_CLEAR(eg->symbols, symbol);
	}
}

void
edge_set_remove_state(struct edge_set **pset, fsm_state_t state)
{
	size_t i;
	struct edge_set *set;

	assert(pset != NULL);
	set = *pset;

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_remove_state: state %d on %p\n",
	    state, (void *)set);
#endif

	if (edge_set_empty(set)) {
		return;
	}

	i = 0;
	while (i < set->count) {
		const struct edge_group *eg = &set->groups[i];
		if (eg->to == state) {
			const size_t to_mv = set->count - i;
			memmove(&set->groups[i],
			    &set->groups[i + 1],
			    to_mv * sizeof(*eg));
			set->count--;
		} else {
			i++;
		}
	}
}

struct to_info {
	fsm_state_t old_to;
	fsm_state_t new_to;
	fsm_state_t assignment;
};

static int
collate_info_by_new_to(const void *pa, const void *pb)
{
	const struct to_info *a = (const struct to_info *)pa;
	const struct to_info *b = (const struct to_info *)pb;
	if (a->new_to < b->new_to) {
		return -1;
	} else if (a->new_to > b->new_to) {
		return 1;
	} else {
		return 0;
	}
}

static int
collate_info_by_old_to(const void *pa, const void *pb)
{
	const struct to_info *a = (const struct to_info *)pa;
	const struct to_info *b = (const struct to_info *)pb;
	if (a->old_to < b->old_to) {
		return -1;
	} else if (a->old_to > b->old_to) {
		return 1;
	} else {
		assert(!"violated uniqueness invariant");
		return 0;
	}
}

#define LOG_COMPACT 0

int
edge_set_compact(struct edge_set **pset, const struct fsm_alloc *alloc,
    fsm_state_remap_fun *remap, const void *opaque)
{
	struct edge_set *set;
	size_t i;
	struct to_info *info;
	struct edge_group *ngroups;
	size_t ncount = 0;

	assert(pset != NULL);
	set = *pset;

#if LOG_BITSET || LOG_COMPACT
	fprintf(stderr, " -- edge_set_compact: set %p\n",
	    (void *)set);
#endif

	if (edge_set_empty(set)) {
		return 1;
	}

	assert(set->count > 0);

	info = f_malloc(alloc, set->count * sizeof(info[0]));
	if (info == NULL) {
		return 0;
	}

	ngroups = f_calloc(alloc, set->ceil, sizeof(ngroups[0]));
	if (ngroups == NULL) {
		f_free(alloc, info);
		return 0;
	}

	/* first pass, construct mapping */
	for (i = 0; i < set->count; i++) {
		struct edge_group *eg = &set->groups[i];
		const fsm_state_t new_to = remap(eg->to, opaque);
#if LOG_BITSET > 1 || LOG_COMPACT
		fprintf(stderr, "compact: %ld, old_to %d -> new_to %d\n",
		    i, eg->to, new_to);
#endif
		info[i].old_to = eg->to;
		info[i].new_to = new_to;
		info[i].assignment = (fsm_state_t)-1; /* not yet assigned */
	}

	/* sort info by new_state */
	qsort(info, set->count, sizeof(info[0]),
	    collate_info_by_new_to);

#if LOG_BITSET > 1 || LOG_COMPACT
	fprintf(stderr, "== after sort by new_state\n");
	for (i = 0; i < set->count; i++) {
		fprintf(stderr, " -- %lu: old_to: %d, new_to: %d, assignment: %d\n",
		    i,
		    info[i].old_to,
		    info[i].new_to,
		    info[i].assignment);
	}
#endif

	info[0].assignment = 0;
	ncount++;

	for (i = 1; i < set->count; i++) {
		const fsm_state_t prev_new_to = info[i - 1].new_to;
		const fsm_state_t prev_assignment = info[i - 1].assignment;

		assert(info[i].new_to >= prev_new_to);

		if (info[i].new_to == FSM_STATE_REMAP_NO_STATE) {
			break;
		}

		if (info[i].new_to == prev_new_to) {
			info[i].assignment = prev_assignment;
		} else {
			info[i].assignment = prev_assignment + 1;
			ncount++;
		}
	}

	/* sort again, by old_state */
	qsort(info, set->count, sizeof(info[0]),
	    collate_info_by_old_to);

#if LOG_BITSET > 1 || LOG_COMPACT
	fprintf(stderr, "== after sort by old_state\n");
	for (i = 0; i < set->count; i++) {
		fprintf(stderr, " -- %lu: old_to: %d, new_to: %d, assignment: %d\n",
		    i,
		    info[i].old_to,
		    info[i].new_to,
		    info[i].assignment);
	}
#endif

	/* second pass, copy/condense */
	for (i = 0; i < set->count; i++) {
		struct edge_group *g;
		size_t w_i;
		if (info[i].new_to == FSM_STATE_REMAP_NO_STATE) {
			continue;
		}
		g = &ngroups[info[i].assignment];
		g->to = info[i].new_to;

		for (w_i = 0; w_i < 256/64; w_i++) {
			g->symbols[w_i] |= set->groups[i].symbols[w_i];
		}
	}

	f_free(alloc, info);
	f_free(alloc, set->groups);
	set->groups = ngroups;
	set->count = ncount;

#if LOG_BITSET > 1 || LOG_COMPACT
	for (i = 0; i < set->count; i++) {
		fprintf(stderr, "ngroups[%zu]: to %d\n",
		    i, set->groups[i].to);
	}
#endif
	return 1;
}

void
edge_set_reset(const struct edge_set *set, struct edge_iter *it)
{
	assert(it != NULL);

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_reset: set %p\n",
	    (void *)set);
#endif

	it->i = 0;
	it->j = 0;
	it->set = set;
}

int
edge_set_next(struct edge_iter *it, struct fsm_edge *e)
{
	const struct edge_set *set;
	assert(it != NULL);
	set = it->set;

	set = it->set;

#if LOG_BITSET > 1
	fprintf(stderr, " -- edge_set_next: set %p, i %ld, j 0x%x\n",
	    (void *)set, it->i, (unsigned)it->j);
#endif

	if (set == NULL) {
		return 0;
	}

	while (it->i < set->count) {
		const struct edge_group *eg = &set->groups[it->i];
		while (it->j < 256) {
			if ((it->j & 63) == 0 && 0 == eg->symbols[it->j/64]) {
				it->j += 64;
			} else {
				if (SYMBOLS_GET(eg->symbols, it->j)) {
					e->symbol = it->j;
					e->state = eg->to;
					it->j++;
					return 1;
				}
				it->j++;
			}
		}

		it->i++;
		it->j = 0;
	}

	return 0;
}

void
edge_set_rebase(struct edge_set **pset, fsm_state_t base)
{
	size_t i;
	struct edge_set *set;

	assert(pset != NULL);
	set = *pset;

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_rebase: set %p, base %d\n",
	    (void *)set, base);
#endif

	if (edge_set_empty(set)) {
		return;
	}

	for (i = 0; i < set->count; i++) {
		struct edge_group *eg = &set->groups[i];
		eg->to += base;
	}
}

int
edge_set_replace_state(struct edge_set **pset, const struct fsm_alloc *alloc,
    fsm_state_t old, fsm_state_t new)
{
	size_t i;
	struct edge_set *set;
	struct edge_group cp;

	assert(pset != NULL);
	set = *pset;

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_replace_state: set %p, state %d -> %d\n",
	    (void *)set, old, new);
#endif

	if (edge_set_empty(set)) {
		return 1;
	}

	/* Invariants: if a group with .to == old appears in the group,
	 * it should only appear once. Replacing .to may lead to
	 * duplicates, so duplicates may need to be merged after. */

	for (i = 0; i < set->count; i++) {
		struct edge_group *eg = &set->groups[i];
		if (eg->to == old) {
			const size_t to_mv = set->count - i;
			unsigned char label;
			if (!find_first_group_label(eg, &label)) {
				return 1; /* ignore empty group */
			}

#if LOG_BITSET
			fprintf(stderr, " -- edge_set_replace_state: removing group at %ld with .to=%d\n", i, old);
#endif
			/* Remove group */
			memcpy(&cp, eg, sizeof(cp));
			memmove(&set->groups[i],
			    &set->groups[i + 1],
			    to_mv * sizeof(set->groups[i]));
			set->count--;

#if LOG_BITSET
			dump_edge_set(set);
			fprintf(stderr, " -- edge_set_replace_state: reinserting group with .to=%d and label 0x%x\n", new, (unsigned)label);
#endif

			/* Realistically, this shouldn't fail, because
			 * edge_set_add only fails on allocation failure
			 * when it needs to grow the backing array, but
			 * we're removing a group and then adding the
			 * group again so add's bookkeeping puts the
			 * group in the appropriate place. */
			if (!edge_set_add(&set, alloc,
				label, new)) {
				return 0;
			}
			dump_edge_set(set);

			for (i = 0; i < set->count; i++) {
				eg = &set->groups[i];
				if (eg->to == new) {
					size_t w_i;
#if LOG_BITSET
					fprintf(stderr, " -- edge_set_replace_state: found new group at %ld, setting other labels from copy\n", i);
#endif
					for (w_i = 0; w_i < 4; w_i++) {
						eg->symbols[w_i] |= cp.symbols[w_i];
					}
					dump_edge_set(set);
					return 1;
				}
			}
			assert(!"internal error: just added, but not found");
		}
	}
	return 1;
}

int
edge_set_empty(const struct edge_set *s)
{
	size_t i;
	if (s == NULL || s->count == 0) {
		return 1;
	}

	for (i = 0; i < s->count; i++) {
		unsigned char label;
		if (find_first_group_label(&s->groups[i], &label)) {
			return 0;
		}
	}

	return 1;
}

void
edge_set_ordered_iter_reset_to(const struct edge_set *set,
    struct edge_ordered_iter *eoi, unsigned char symbol)
{
	eoi->symbol = symbol;		/* stride by character */
	eoi->pos = 0;
	eoi->set = set;

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_ordered_iter_reset_to: set %p, symbol 0x%x\n",
	    (void *)set, symbol);
#endif

}

/* Reset an ordered iterator, equivalent to
 * edge_set_ordered_iter_reset_to(set, eoi, '\0'). */
void
edge_set_ordered_iter_reset(const struct edge_set *set,
    struct edge_ordered_iter *eoi)
{
	edge_set_ordered_iter_reset_to(set, eoi, 0x00);
}

/* Get the next edge from an ordered iterator and return 1,
 * or return 0 when no more are available. */
int
edge_set_ordered_iter_next(struct edge_ordered_iter *eoi, struct fsm_edge *e)
{
	const struct edge_set *set;
	assert(eoi != NULL);

	set = eoi->set;

#if LOG_BITSET
	fprintf(stderr, " -- edge_set_ordered_iter_next: set %p, pos %ld, symbol 0x%x\n",
	    (void *)set, eoi->pos, eoi->symbol);
#endif

	if (set == NULL) {
		return 0;
	}

	for (;;) {
		while (eoi->pos < set->count) {
			struct edge_group *eg = &set->groups[eoi->pos++];
			if (SYMBOLS_GET(eg->symbols, eoi->symbol)) {
				e->symbol = eoi->symbol;
				e->state = eg->to;
				return 1;
			}
		}

		if (eoi->symbol == 255) { /* done */
			eoi->set = NULL;
			return 0;
		} else {
			eoi->symbol++;
			eoi->pos = 0;
		}
	}

	return 0;
}

void
edge_set_group_iter_reset(const struct edge_set *set,
    enum edge_group_iter_type iter_type,
    struct edge_group_iter *egi)
{
	memset(egi, 0x00, sizeof(*egi));
	egi->set = set;
	egi->flag = iter_type;

#if LOG_BITSET > 1
	fprintf(stderr, " -- edge_set_group_iter_reset: set %p, type %d\n",
	    (void *)set, iter_type);
#endif

	if (iter_type == EDGE_GROUP_ITER_UNIQUE && set != NULL) {
		struct edge_group *g;
		size_t g_i, i;
		uint64_t seen[256/64] = { 0 };
		for (g_i = 0; g_i < set->count; g_i++) {
			g = &set->groups[g_i];
			for (i = 0; i < 256; i++) {
				if ((i & 63) == 0 && g->symbols[i/64] == 0) {
					i += 63; /* skip empty word */
					continue;
				}
				if (SYMBOLS_GET(g->symbols, i)) {
					if (SYMBOLS_GET(seen, i)) {
						SYMBOLS_SET(egi->internal, i);
					} else {
						SYMBOLS_SET(seen, i);
					}
				}
			}
		}
	}
}

int
edge_set_group_iter_next(struct edge_group_iter *egi,
    struct edge_group_iter_info *eg)
{
	struct edge_group *g;
	int any = 0;
	size_t i;
advance:
	if (egi->set == NULL || egi->i == egi->set->count) {
#if LOG_BITSET > 1
		fprintf(stderr, " -- edge_set_group_iter_next: set %p, count %lu, done\n",
		    (void *)egi->set, egi->i);
#endif
		return 0;
	}

	g = &egi->set->groups[egi->i];

	eg->to = g->to;

#if LOG_BITSET > 1
	fprintf(stderr, " -- edge_set_group_iter_next: flag %d, i %zu, to %d\n",
	    egi->flag, egi->i, g->to);
#endif

	if (egi->flag == EDGE_GROUP_ITER_ALL) {
		egi->i++;
		for (i = 0; i < 4; i++) {
			eg->symbols[i] = g->symbols[i];
			if (eg->symbols[i] != 0) {
				any = 1;
			}
		}
		if (!any) {
			goto advance;
		}
		return 1;
	} else if (egi->flag == EDGE_GROUP_ITER_UNIQUE) { /* uniques first */
		for (i = 0; i < 4; i++) {
			eg->symbols[i] = g->symbols[i] &~ egi->internal[i];
			if (eg->symbols[i] != 0) {
				any = 1;
			}
		}

		/* next time, yield non-uniques */
		egi->flag = EDGE_GROUP_ITER_UNIQUE + 1;

		/* if there are any uniques, yield them, otherwise
		 * continue to the non-unique branch below. */
		if (any) {
			eg->unique = 1;
			return 1;
		}
	}

        if (egi->flag == EDGE_GROUP_ITER_UNIQUE + 1) {
		for (i = 0; i < 4; i++) {
			eg->symbols[i] = g->symbols[i] & egi->internal[i];
			if (eg->symbols[i]) {
				any = 1;
			}
		}
		eg->unique = 0;

		egi->flag = EDGE_GROUP_ITER_UNIQUE;
		egi->i++;
		if (!any) {
			goto advance;
		}

		return 1;
	} else {
		assert("match fail");
		return 0;
	}
}

#endif
