/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
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

	assert(set->a != NULL);

	free(set->a);
	free(set);
}

int
edge_set_add(struct edge_set **setp, const struct fsm_alloc *alloc,
	unsigned char symbol, fsm_state_t state)
{
	struct edge_set *set;

	assert(setp != NULL);

	if (*setp == NULL) {
		*setp = edge_set_create(alloc);
		if (*setp == NULL) {
			return 0;
		}

		/* fallthrough */
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
edge_set_contains(const struct edge_set *set, unsigned char symbol)
{
	size_t i;

	if (edge_set_empty(set)) {
		return 0;
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
	assert(src != NULL);

	for (edge_set_reset((void *) src, &jt); edge_set_next(&jt, &e); ) {
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

	set = *setp;

	if (edge_set_empty(set)) {
		return;
	}

	for (i = 0; i < set->i; i++) {
		if (set->a[i].symbol == symbol) {
			memmove(&set->a[i], &set->a[i + 1], (set->i - i - 1) * (sizeof *set->a));
			set->i--;
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

	set = *setp;

	if (edge_set_empty(set)) {
		return;
	}

	for (i = 0; i < set->i; i++) {
		if (set->a[i].state == state) {
			memmove(&set->a[i], &set->a[i + 1], (set->i - i - 1) * (sizeof *set->a));
			set->i--;
		}
	}
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

	return set->i == 0;
}

