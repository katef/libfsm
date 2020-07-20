/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "libfsm/internal.h" /* XXX: for allocating struct fsm_edge, and the edges array */

#include <print/esc.h>

#include <adt/alloc.h>
#include <adt/bitmap.h>
#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#define SET_INITIAL 8

/*
 * Many edge sets only contain a single item, as is the case for state sets.
 * Likewise we avoid allocating for an edge set until it contains more than
 * a single item. As with state sets, the single tem is stored within the
 * directly in the edge set pointer value. I would draw the layout here,
 * but I'm not feeling very artistic today.
 */
#define SINGLETON_MAX_STATE             ((~ (uintptr_t) 0U) >> (CHAR_BIT + 1))
#define SINGLETON_ENCODE(symbol, state) ((void *) ( \
                                        	(((uintptr_t) (state)) << (CHAR_BIT + 1)) | \
                                        	(((uintptr_t) (symbol)) << 1) | \
                                        	0x1))
#define SINGLETON_DECODE_SYMBOL(ptr)    ((unsigned char) ((((uintptr_t) (ptr)) >> 1) & 0xFFU))
#define SINGLETON_DECODE_STATE(ptr)     ((fsm_state_t)   (((uintptr_t) (ptr)) >> (CHAR_BIT + 1)))
#define IS_SINGLETON(ptr)               (((uintptr_t) (ptr)) & 0x1)

struct edge_set {
	const struct fsm_alloc *alloc;
	struct fsm_edge *a;
	size_t i;
	size_t n;
};

static struct edge_set *
edge_set_create(const struct fsm_alloc *a)
{
	struct edge_set *set;

	set = f_malloc(a, sizeof *set);
	if (set == NULL) {
		return NULL;
	}

	set->a = f_malloc(a, SET_INITIAL * sizeof *set->a);
	if (set->a == NULL) {
		f_free(a, set);
		return NULL;
	}

	set->alloc = a;
	set->i = 0;
	set->n = SET_INITIAL;

	return set;
}

void
edge_set_free(struct edge_set *set)
{
	if (set == NULL) {
		return;
	}

	if (IS_SINGLETON(set)) {
		return;
	}

	assert(set->a != NULL);

	f_free(set->alloc, set->a);
	f_free(set->alloc, set);
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

	if (set->i == set->n) {
		struct fsm_edge *new;

		new = f_realloc(set->alloc, set->a, (sizeof *set->a) * (set->n * 2));
		if (new == NULL) {
			return 0;
		}

		set->a = new;
		set->n *= 2;
	}

	set->a[set->i].symbol = symbol;
	set->a[set->i].state  = state;

	set->i++;

	assert(edge_set_contains(set, symbol));

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
edge_set_contains(const struct edge_set *set, unsigned char symbol)
{
	size_t i;

	if (edge_set_empty(set)) {
		return 0;
	}

	if (IS_SINGLETON(set)) {
		return SINGLETON_DECODE_SYMBOL(set) == symbol;
	}

	assert(set != NULL);

	for (i = 0; i < set->i; i++) {
		if (set->a[i].symbol == symbol) {
			return 1;
		}
	}

	return 0;
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

	for (i = 0; i < set->i; i++) {
		if (bm_get(bm, set->a[i].symbol)) {
			return 1;
		}

		bm_set(bm, set->a[i].symbol);
	}

	return 0;
}

int
edge_set_transition(const struct edge_set *set, unsigned char symbol,
	fsm_state_t *state)
{
	size_t i;
#ifndef NDEBUG
	struct bm bm;
#endif

	assert(state != NULL);

	/*
	 * This function is meaningful for DFA only; we require a DFA
	 * by contract in order to identify a single destination state
	 * for a given symbol.
	 */
#ifndef NDEBUG
	bm_clear(&bm);
	assert(!edge_set_hasnondeterminism(set, &bm));
#endif

	if (edge_set_empty(set)) {
		return 0;
	}

	if (IS_SINGLETON(set)) {
		if (SINGLETON_DECODE_SYMBOL(set) == symbol) {
			*state = SINGLETON_DECODE_STATE(set);
			return 1;
		}

		return 0;
	}

	assert(set != NULL);

	for (i = 0; i < set->i; i++) {
		if (set->a[i].symbol == symbol) {
			*state = set->a[i].state;
			return 1;
		}
	}

	return 0;
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

	assert(set->a != NULL);

	return set->i;
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

	for (edge_set_reset((void *) src, &jt); edge_set_next(&jt, &e); ) {
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
	size_t i;

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
	}

	i = 0;
	while (i < set->i) {
		if (set->a[i].symbol == symbol) {
			if (i < set->i) {
				memmove(&set->a[i], &set->a[i + 1], (set->i - i - 1) * (sizeof *set->a));
			}
			set->i--;
		} else {
			i++;
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

	i = 0;
	while (i < set->i) {
		if (set->a[i].state == state) {
			if (i < set->i) {
				memmove(&set->a[i], &set->a[i + 1], (set->i - i - 1) * (sizeof *set->a));
			}
			set->i--;
		} else {
			i++;
		}
	}
}

void
edge_set_compact(struct edge_set **setp,
    fsm_state_remap_fun *remap, void *opaque)
{
	struct edge_set *set;
	size_t i, removed, dst;

	assert(setp != NULL);

	if (IS_SINGLETON(*setp)) {
		const unsigned char symbol = SINGLETON_DECODE_SYMBOL(*setp);
		const fsm_state_t s = SINGLETON_DECODE_STATE(*setp);
		const fsm_state_t new_id = remap(s, opaque);
		if (new_id == FSM_STATE_REMAP_NO_STATE) {
			*setp = NULL;
		} else {
			assert(new_id <= s);
			*setp = SINGLETON_ENCODE(symbol, new_id);
		}
		return;
	}

	set = *setp;

	if (edge_set_empty(set)) {
		return;
	}

	i = 0;
	removed = 0;
	dst = 0;
	for (i = 0; i < set->i; i++) {
		const fsm_state_t to = set->a[i].state;
		const fsm_state_t new_to = remap(to, opaque);

		if (new_to == FSM_STATE_REMAP_NO_STATE) { /* drop */
			removed++;
		} else {	/* keep */
			if (dst < i) {
				memcpy(&set->a[dst],
				    &set->a[i],
				    sizeof(set->a[i]));
			}
			set->a[dst].state = new_to;
			dst++;
		}
	}

	set->i -= removed;
	assert(set->i == dst);
}

void
edge_set_reset(struct edge_set *set, struct edge_iter *it)
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

	if (it->i >= it->set->i) {
		return 0;
	}

	*e = it->set->a[it->i];

	it->i++;

	return 1;
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

	for (i = 0; i < set->i; i++) {
		set->a[i].state += base;
	}
}

void
edge_set_replace_state(struct edge_set **setp, fsm_state_t old, fsm_state_t new)
{
	struct edge_set *set;
	size_t i;

	assert(setp != NULL);

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

	for (i = 0; i < set->i; i++) {
		if (set->a[i].state == old) {
			set->a[i].state = new;
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

	return set->i == 0;
}

