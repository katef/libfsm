/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

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

void
edge_set_compact(struct edge_set **setp,
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
		return;
	}

	set = *setp;

	if (edge_set_empty(set)) {
		return;
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

void
edge_set_replace_state(struct edge_set **setp, fsm_state_t old, fsm_state_t new)
{
	struct edge_set *set;
	size_t i;

	assert(setp != NULL);
	assert(old != BUCKET_UNUSED);
	assert(old != BUCKET_TOMBSTONE);

	if (IS_SINGLETON(*setp)) {
		if (SINGLETON_DECODE_STATE(*setp) == old) {
			unsigned char symbol;

			symbol = SINGLETON_DECODE_SYMBOL(*setp);

			*setp = SINGLETON_ENCODE(symbol, new);
			assert(SINGLETON_DECODE_SYMBOL(*setp) == symbol);
			assert(SINGLETON_DECODE_STATE(*setp) == new);
		}
		return;
	}

	set = *setp;

	if (edge_set_empty(set)) {
		return;
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
