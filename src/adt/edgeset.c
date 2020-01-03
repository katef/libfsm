/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
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

struct edge_set *
edge_set_create(const struct fsm_alloc *a)
{
	struct edge_set *set;

	set = f_malloc(a, sizeof *set);
	if (set == NULL) {
		return NULL;
	}

	set->a = f_malloc(a, SET_INITIAL * sizeof *set->a);
	if (set->a == NULL) {
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

struct fsm_edge *
edge_set_add(struct edge_set *set, unsigned char symbol,
	fsm_state_t state)
{
	struct fsm_edge *e;

	assert(set != NULL);

	if (set->i == set->n) {
		struct fsm_edge *new;

		new = f_realloc(set->alloc, set->a, (sizeof *set->a) * (set->n * 2));
		if (new == NULL) {
			return NULL;
		}

		set->a = new;
		set->n *= 2;
	}

	e = &set->a[set->i];

	e->symbol = symbol;
	e->state  = state;

	set->i++;

	assert(edge_set_contains(set, symbol));

	return e;
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

	for (i = 0; i < set->i; i++) {
		/* 
		 * Instances of struct fsm_edge aren't unique, and are not ordered.
		 * The bitmap here is to identify duplicate symbols between structs.
		 * 
		 * The same bitmap is shared between all states in an epsilon closure.
		 */
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

	assert(set != NULL);
	assert(set->a != NULL);

	return set->i;
}

int
edge_set_copy(struct edge_set *dst, const struct edge_set *src)
{
	struct edge_iter jt;
	struct fsm_edge *e;

	assert(dst != NULL);
	assert(src != NULL);

	for (e = edge_set_first((void *) src, &jt); e != NULL; e = edge_set_next(&jt)) {
		if (!edge_set_add(dst, e->symbol, e->state)) {
			return 0;
		}
	}

	return 1;
}

void
edge_set_remove(struct edge_set *set, unsigned char symbol)
{
	size_t i;

	assert(set != NULL);

	if (set == NULL) {
		return;
	}

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
edge_set_remove_state(struct edge_set *set, fsm_state_t state)
{
	size_t i;

	assert(set != NULL);

	if (set == NULL) {
		return;
	}

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

struct fsm_edge *
edge_set_first(struct edge_set *set, struct edge_iter *it)
{
	assert(it != NULL);

	if (set == NULL) {
		return NULL;
	}

	assert(set != NULL);
	assert(set->a != NULL);

	if (edge_set_empty(set)) {
		it->set = NULL;
		return NULL;
	}

	it->i = 0;
	it->set = set;

	return &it->set->a[it->i];
}

struct fsm_edge *
edge_set_next(struct edge_iter *it)
{
	assert(it != NULL);

	it->i++;
	if (it->i >= it->set->i) {
		return NULL;
	}

	return &it->set->a[it->i];
}

int
edge_set_empty(const struct edge_set *set)
{
	if (set == NULL) {
		return 1;
	}

	return set->i == 0;
}

