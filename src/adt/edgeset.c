/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "libfsm/internal.h" /* XXX: for allocating struct fsm_edge */

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
	int (*cmp)(const void *, const void *);
};

static int
set_empty(const struct edge_set *set)
{
	assert(set != NULL);

	return set->i == 0;
}

/*
 * Return where an item would be, if it were inserted
 */
static size_t
set_search(const struct edge_set *set, const struct fsm_edge *item)
{
	size_t start, end;
	size_t mid;

	assert(set != NULL);
	assert(set->cmp != NULL);
	assert(item != NULL);

	start = mid = 0;
	end = set->i;

	while (start < end) {
		int r;
		mid = start + (end - start) / 2;
		r = set->cmp(&item, &set->a[mid]);
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

static struct fsm_edge *
set_contains(const struct edge_set *set, const struct fsm_edge *item)
{
	size_t i;

	assert(set != NULL);
	assert(set->cmp != NULL);
	assert(item != NULL);

	if (set_empty(set)) {
		return NULL;
	}

	i = set_search(set, item);
	if (set->cmp(&item, &set->a[i]) == 0) {
		return set->a[i];
	}

	return NULL;
}

static struct edge_set *
set_create(const struct fsm_alloc *a,
	int (*cmp)(const void *a, const void *b))
{
	struct edge_set *set;

	assert(cmp != NULL);

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
	set->cmp = cmp;

	return set;
}

static struct fsm_edge *
set_add(struct edge_set *set, struct fsm_edge *item)
{
	size_t i;

	assert(set != NULL);
	assert(set->cmp != NULL);
	assert(item != NULL);

	i = 0;

	/*
	 * If the item already exists in the set, return success.
	 */
	if (!set_empty(set)) {
		i = set_search(set, item);
		if (set->cmp(&item, &set->a[i]) == 0) {
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

		if (set->cmp(&item, &set->a[i]) > 0) {
			i++;
		}

		memmove(&set->a[i + 1], &set->a[i], (set->i - i) * (sizeof *set->a));
		set->a[i] = item;
		set->i++;
	} else {
		set->a[0] = item;
		set->i = 1;
	}

	assert(set_contains(set, item));

	return item;
}

static void
set_remove(struct edge_set *set, const struct fsm_edge *item)
{
	size_t i;

	assert(set != NULL);
	assert(set->cmp != NULL);
	assert(item != NULL);

	if (set_empty(set)) {
		return;
	}

	i = set_search(set, item);
	if (set->cmp(&item, &set->a[i]) == 0) {
		if (i < set->i) {
			memmove(&set->a[i], &set->a[i + 1], (set->i - i - 1) * (sizeof *set->a));
		}

		set->i--;
	}

	assert(!set_contains(set, item));
}

static void
set_free(struct edge_set *set)
{
	assert(set != NULL);
	assert(set->a != NULL);

	free(set->a);
	free(set);
}

static size_t
set_count(const struct edge_set *set)
{
	assert(set != NULL);
	assert(set->a != NULL);

	return set->i;
}

static struct fsm_edge *
set_first(const struct edge_set *set, struct edge_iter *it)
{
	assert(set != NULL);
	assert(set->a != NULL);
	assert(it != NULL);

	if (set_empty(set)) {
		it->set = NULL;
		return NULL;
	}

	it->i = 0;
	it->set = set;

	return it->set->a[it->i];
}

static struct fsm_edge *
set_firstafter(const struct edge_set *set, struct edge_iter *it, const struct fsm_edge *item)
{
	size_t i;
	int r;

	assert(set != NULL);
	assert(set->cmp != NULL);
	assert(set->a != NULL);
	assert(it != NULL);

	if (set_empty(set)) {
		it->set = NULL;
		return NULL;
	}

	i = set_search(set, item);
	r = set->cmp(&item, &set->a[i]);
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

static struct fsm_edge *
set_next(struct edge_iter *it)
{
	assert(it != NULL);

	it->i++;
	if (it->i >= it->set->i) {
		return NULL;
	}

	return it->set->a[it->i];
}

static struct fsm_edge *
set_only(const struct edge_set *set)
{
	assert(set != NULL);
	assert(set->n >= 1);
	assert(set->i == 1);
	assert(set->a[0] != NULL);

	return set->a[0];
}

static int
set_hasnext(const struct edge_iter *it)
{
	assert(it != NULL);

	return it->set && it->i + 1 < it->set->i;
}

struct edge_set *
edge_set_create(const struct fsm_alloc *a,
	int (*cmp)(const void *a, const void *b))
{
	assert(cmp != NULL);

	return set_create(a, cmp);
}

void
edge_set_free(struct edge_set *set)
{
	if (set == NULL) {
		return;
	}

	set_free(set);
}

struct fsm_edge *
edge_set_add(struct edge_set *set, struct fsm_edge *e)
{
	assert(set != NULL);
	assert(e != NULL);

	return set_add(set, e);
}

struct fsm_edge *
edge_set_contains(const struct edge_set *set, const struct fsm_edge *e)
{
	assert(set != NULL);
	assert(e != NULL);

	return set_contains(set, e);
}

size_t
edge_set_count(const struct edge_set *set)
{
	if (set == NULL) {
		return 0;
	}

	return set_count(set);
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
edge_set_remove(struct edge_set *set, const struct fsm_edge *e)
{
	assert(e != NULL);

	if (set == NULL) {
		return;
	}

	set_remove(set, e);
}

struct fsm_edge *
edge_set_first(struct edge_set *set, struct edge_iter *it)
{
	assert(it != NULL);

	if (set == NULL) {
		return NULL;
	}

	return set_first(set, it);
}

struct fsm_edge *
edge_set_firstafter(const struct edge_set *set, struct edge_iter *it, const struct fsm_edge *e)
{
	assert(set != NULL);
	assert(it != NULL);
	assert(e != NULL);

	return set_firstafter(set, it, e);
}

struct fsm_edge *
edge_set_next(struct edge_iter *it)
{
	assert(it != NULL);

	return set_next(it);
}

int
edge_set_hasnext(struct edge_iter *it)
{
	assert(it != NULL);

	return set_hasnext(it);
}

int
edge_set_empty(const struct edge_set *set)
{
	if (set == NULL) {
		return 1;
	}

	return set_empty(set);
}

struct fsm_edge *
edge_set_only(const struct edge_set *set)
{
	assert(set != NULL);

	return set_only(set);
}

