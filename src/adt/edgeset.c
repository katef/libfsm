/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "libfsm/internal.h" /* XXX: for allocating struct fsm_edge, and the edges array */

#include <adt/alloc.h>
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
edge_set_add(struct edge_set *set, unsigned char symbol)
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

	set->a[set->i].symbol = symbol;
	set->a[set->i].sl     = NULL;
	e = &set->a[set->i];
	set->i++;

	assert(edge_set_contains(set, symbol));

	return e;
}

struct fsm_edge *
edge_set_contains(const struct edge_set *set, unsigned char symbol)
{
	size_t i;

	if (edge_set_empty(set)) {
		return NULL;
	}

	assert(set != NULL);

	for (i = 0; i < set->i; i++) {
		if (set->a[i].symbol == symbol) {
			return &set->a[i];
		}
	}

	return NULL;
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
edge_set_copy(struct edge_set *dst, const struct fsm_alloc *alloc,
	const struct edge_set *src)
{
	struct edge_iter jt;
	struct fsm_edge *e;

	assert(dst != NULL);
	assert(src != NULL);

	for (e = edge_set_first((void *) src, &jt); e != NULL; e = edge_set_next(&jt)) {
		struct fsm_edge *en;

		en = edge_set_add(dst, e->symbol);
		if (en == NULL) {
			return 0;
		}

		if (!state_set_copy(&en->sl, alloc, e->sl)) {
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
edge_set_hasnext(struct edge_iter *it)
{
	assert(it != NULL);

	return it->set && it->i + 1 < it->set->i;
}

int
edge_set_empty(const struct edge_set *set)
{
	if (set == NULL) {
		return 1;
	}

	return set->i == 0;
}

struct fsm_edge *
edge_set_only(const struct edge_set *set)
{
	assert(set != NULL);
	assert(set->n >= 1);
	assert(set->i == 1);

	return &set->a[0];
}

