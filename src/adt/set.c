/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <adt/alloc.h>
#include <adt/set.h>

#define SET_INITIAL 8

struct set {
	const struct fsm_alloc *alloc;
	void **a;
	size_t i;
	size_t n;
	int (*cmp)(const void *, const void *);
};

/*
 * Return where an item would be, if it were inserted
 */
static size_t
set_search(const struct set *set, const void *item)
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
		r = set->cmp(item, set->a[mid]);
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

static int
set_cmpval(const void *a, const void *b)
{
	return (a > b) - (a < b);
}

struct set *
set_create(const struct fsm_alloc *a,
	int (*cmp)(const void *a, const void *b))
{
	struct set *set;

	if (cmp == NULL) {
		cmp = set_cmpval;
	}

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

static int
set_bulkcmpval(const void *a, const void *b)
{
	const void *pa = ((void **)a)[0], *pb = ((void **)b)[0];

	return (pa > pb) - (pa < pb);
}

struct set *
set_create_singleton(const struct fsm_alloc *a,
	int (*cmp)(const void *a, const void *b), void *item)
{
	struct set *s;

	s = f_malloc(a, sizeof *s);
	if (s == NULL) {
		return NULL;
	}

	s->a = f_malloc(a, sizeof *s->a);
	if (s->a == NULL) {
		f_free(a, s);
		return NULL;
	}

	s->alloc = a;
	s->a[0] = item;
	s->i = s->n = 1;
	s->cmp = (cmp != NULL) ? cmp : set_cmpval;

	return s;
}

struct set *
set_create_from_array(const struct fsm_alloc *a,
	void *items[], size_t n,
	int (*cmp)(const void *a, const void *b),
	int (*bulkcmp)(const void *, const void *))
{
	struct set *s;

	if (n == 0) {
		return set_create(a, cmp);
	}

	s = f_malloc(a, sizeof *s);
	if (s == NULL) {
		return NULL;
	}

	s->cmp = (cmp != NULL) ? cmp : set_cmpval;

	if (n > 1) {
		/* XXX - replace with something better? */
		qsort((void *)(&items[0]), n, sizeof items[0], (bulkcmp != NULL) ? bulkcmp : set_bulkcmpval);
	}

	s->alloc = a;
	s->i = n;
	s->n = n;
	s->a = items;

	return s;
}

struct set *
set_copy(const struct set *set)
{
	struct set *s;
	s = malloc(sizeof *s);
	if (s == NULL) {
		return NULL;
	}

	s->cmp = set->cmp;
	s->a = malloc(set->i * sizeof s->a[0]);
	if (s->a == NULL) {
		free(s);
		return NULL;
	}

	s->i = s->n = set->i;
	memcpy(s->a, set->a, s->n * sizeof s->a[0]);

	return s;
}

void *
set_add(struct set *set, void *item)
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
		if (set->cmp(item, set->a[i]) == 0) {
			return item;
		}
	}

	if (set->i) {
		/* We're at capacity. Get more */
		if (set->i == set->n) {
			void **new;

			new = f_realloc(set->alloc, set->a, (sizeof *set->a) * (set->n * 2));
			if (new == NULL) {
				return NULL;
			}

			set->a = new;
			set->n *= 2;
		}

		if (set->cmp(item, set->a[i]) > 0) {
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

void
set_remove(struct set *set, const void *item)
{
	size_t i;

	assert(set != NULL);
	assert(set->cmp != NULL);
	assert(item != NULL);

	if (set_empty(set)) {
		return;
	}

	i = set_search(set, item);
	if (set->cmp(item, set->a[i]) == 0) {
		if (i < set->i) {
			memmove(&set->a[i], &set->a[i + 1], (set->i - i - 1) * (sizeof *set->a));
		}

		set->i--;
	}

	assert(!set_contains(set, item));
}

void
set_free(struct set *set)
{
	assert(set != NULL);
	assert(set->a != NULL);

	free(set->a);
	free(set);
}

size_t
set_count(const struct set *set)
{
	assert(set != NULL);
	assert(set->a != NULL);

	return set->i;
}

void
set_clear(struct set *set)
{
	assert(set != NULL);
	assert(set->a != NULL);

	set->i = 0;
}

void *
set_contains(const struct set *set, const void *item)
{
	size_t i;

	assert(set != NULL);
	assert(set->cmp != NULL);
	assert(item != NULL);

	if (set_empty(set)) {
		return NULL;
	}

	i = set_search(set, item);
	if (set->cmp(item, set->a[i]) == 0) {
		return set->a[i];
	}

	return NULL;
}

int
set_cmp(const struct set *a, const struct set *b)
{
	assert(a != NULL);
	assert(a->a != NULL);
	assert(b != NULL);
	assert(b->a != NULL);

	if (a->i != b->i) {
		return a->i - b->i;
	}

	return memcmp(a->a, b->a, a->i * sizeof *a->a);
}

int
set_equal(const struct set *a, const struct set *b)
{
	assert(a != NULL);
	assert(a->a != NULL);
	assert(b != NULL);
	assert(b->a != NULL);

	if (a->i != b->i) {
		return 0;
	}

	return 0 == memcmp(a->a, b->a, a->i * sizeof *a->a);
}

int
set_empty(const struct set *set)
{
	assert(set != NULL);

	return set->i == 0;
}

void *
set_first(const struct set *set, struct set_iter *it)
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

void *
set_firstafter(const struct set *set, struct set_iter *it, const void *item)
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
	r = set->cmp(item, set->a[i]);
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

void *
set_next(struct set_iter *it)
{
	assert(it != NULL);

	it->i++;
	if (it->i >= it->set->i) {
		return NULL;
	}

	return it->set->a[it->i];
}

void *
set_only(const struct set *set)
{
	assert(set != NULL);
	assert(set->n >= 1);
	assert(set->i == 1);
	assert(set->a[0] != NULL);

	return set->a[0];
}

int
set_hasnext(const struct set_iter *it)
{
	assert(it != NULL);

	return it->set && it->i + 1 < it->set->i;
}

const void **
set_array(const struct set *set)
{
	if (set == NULL) {
		return 0;
	}

	return (const void **) set->a;
}

