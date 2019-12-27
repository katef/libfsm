/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "libfsm/internal.h" /* XXX: for allocating struct fsm_edge, and cmp */

#include <adt/alloc.h>
#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#define SET_INITIAL 8

struct edge_set {
	const struct fsm_alloc *alloc;
	struct fsm_edge **a;
	size_t i;
	size_t n;
};

static int
cmp(const struct fsm_edge *a, const struct fsm_edge *b)
{
	assert(a != NULL);
	assert(b != NULL);

	/*
	 * N.B. various edge iterations rely on the ordering of edges to be in
	 * ascending order.
	 */
	return (a->symbol > b->symbol) - (a->symbol < b->symbol);
}

/*
 * Return where an item would be, if it were inserted
 */
static size_t
edge_set_search(const struct edge_set *set, const struct fsm_edge *item)
{
	size_t start, end;
	size_t mid;

	assert(set != NULL);
	assert(item != NULL);

	start = mid = 0;
	end = set->i;

	while (start < end) {
		int r;
		mid = start + (end - start) / 2;
		r = cmp(item, set->a[mid]);
		if (r < 0) {
			end = mid;
		} else if (r > 0) {
			start = mid + 1;
		} else {
			return mid;
		}
	}

	return mid;
}

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
edge_set_add(struct edge_set *set, struct fsm_edge *item)
{
	size_t i;

	assert(set != NULL);
	assert(item != NULL);

	i = 0;

	/*
	 * If the item already exists in the set, return success.
	 */
	if (!edge_set_empty(set)) {
		i = edge_set_search(set, item);
		if (cmp(item, set->a[i]) == 0) {
			return item;
		}
	}

	if (set->i) {
		/* We're at capacity. Get more */
		if (set->i == set->n) {
			struct fsm_edge **new;

			new = f_realloc(set->alloc, set->a, (sizeof *set->a) * (set->n * 2));
			if (new == NULL) {
				return NULL;
			}

			set->a = new;
			set->n *= 2;
		}

		if (cmp(item, set->a[i]) > 0) {
			i++;
		}

		memmove(&set->a[i + 1], &set->a[i], (set->i - i) * (sizeof *set->a));
		set->a[i] = item;
		set->i++;
	} else {
		set->a[0] = item;
		set->i = 1;
	}

	assert(edge_set_contains(set, item));

	return item;
}

struct fsm_edge *
edge_set_contains(const struct edge_set *set, const struct fsm_edge *item)
{
	size_t i;

	if (edge_set_empty(set)) {
		return NULL;
	}

	assert(set != NULL);
	assert(item != NULL);

	i = edge_set_search(set, item);
	if (cmp(item, set->a[i]) == 0) {
		return set->a[i];
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

		en = edge_set_contains(dst, e);
		if (en == NULL) {
			en = f_malloc(alloc, sizeof *en);
			if (en == NULL) {
				return 0;
			}

			en->symbol = e->symbol;
			en->sl = NULL;

			if (!edge_set_add(dst, en)) {
				return 0;
			}
		}

		if (!state_set_copy(&en->sl, alloc, e->sl)) {
			return 0;
		}   
	}

	return 1;
}

void
edge_set_remove(struct edge_set *set, const struct fsm_edge *item)
{
	size_t i;

	assert(set != NULL);
	assert(item != NULL);

	if (set == NULL) {
		return;
	}

	if (edge_set_empty(set)) {
		return;
	}

	i = edge_set_search(set, item);
	if (cmp(item, set->a[i]) == 0) {
		if (i < set->i) {
			memmove(&set->a[i], &set->a[i + 1], (set->i - i - 1) * (sizeof *set->a));
		}

		set->i--;
	}

	assert(!edge_set_contains(set, item));
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

	return it->set->a[it->i];
}

struct fsm_edge *
edge_set_firstafter(const struct edge_set *set, struct edge_iter *it, const struct fsm_edge *item)
{
	size_t i;
	int r;

	assert(set != NULL);
	assert(set->a != NULL);
	assert(it != NULL);
	assert(item != NULL);

	if (edge_set_empty(set)) {
		it->set = NULL;
		return NULL;
	}

	i = edge_set_search(set, item);
	r = cmp(item, set->a[i]);
	assert(i <= set->i - 1);

	if (r >= 0 && i == set->i - 1) {
		it->set = NULL;
		return NULL;
	}

	it->i = i;
	if (r >= 0) {
		it->i++;
	}

	it->set = set;
	return it->set->a[it->i];
}

struct fsm_edge *
edge_set_next(struct edge_iter *it)
{
	assert(it != NULL);

	it->i++;
	if (it->i >= it->set->i) {
		return NULL;
	}

	return it->set->a[it->i];
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
	assert(set->a[0] != NULL);

	return set->a[0];
}

