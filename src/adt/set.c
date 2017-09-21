/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <adt/set.h>

#define SET_INITIAL 8

struct set {
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

	assert(item != NULL);
	assert(set != NULL);
	assert(set->cmp != NULL);

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
set_create(int (*cmp)(const void *a, const void *b))
{
	struct set *s;

	assert(cmp != NULL);

	s = malloc(sizeof *s);
	if (s == NULL) {
		return NULL;
	}

	s->a = malloc(SET_INITIAL * sizeof *s->a);
	if (s->a == NULL) {
		return NULL;
	}

	s->i = 0;
	s->n = SET_INITIAL;
	s->cmp = cmp;

	return s;
}

void *
set_add(struct set **set, void *item)
{
	struct set *s;
	size_t i;

	assert(set != NULL);
	assert(item != NULL);

	s = *set;
	i = 0;

	/*
	 * If the set is not initialized, go ahead and do that with the
	 * default comparison function and insert the new item at the front.
	 */
	if (s == NULL) {
		s = set_create(set_cmpval);
		s->a[0] = item;
		s->i = 1;

		*set = s;

		assert(set_contains(*set, item));

		return item;
	}

	assert(s->cmp != NULL);

	/*
	 * If the item already exists in the set, return success.
	 *
	 * TODO: Notify on success differently somehow when the item
	 * was already there, than if we successfully inserted
	 * a non-existing item.
	 */
	if (!set_empty(s)) {
		i = set_search(s, item);
		if (s->cmp(item, s->a[i]) == 0) {
			return item;
		}
	}

	if (s->i) {
		/* We're at capacity. Get more */
		if (s->i == s->n) {
			void **new;

			new = realloc(s->a, (sizeof *s->a) * (s->n * 2));
			if (new == NULL) {
				return NULL;
			}

			s->a = new;
			s->n *= 2;
		}

		if (s->cmp(item, s->a[i]) > 0) {
			i++;
		}

		memmove(&s->a[i + 1], &s->a[i], (s->i - i) * (sizeof *s->a));
		s->a[i] = item;
		s->i++;
	} else {
		s->a[0] = item;
		s->i = 1;
	}

	assert(set_contains(s, item));

	return item;
}

void
set_remove(struct set **set, void *item)
{
	struct set *s = *set;
	size_t i;

	assert(item != NULL);
	assert(s->cmp != NULL);

	if (set_empty(s)) {
		return;
	}

	i = set_search(s, item);
	if (s->cmp(item, s->a[i]) == 0) {
		if (i < s->i) {
			memmove(&s->a[i], &s->a[i + 1], (s->i - i - 1) * (sizeof *s->a));
		}

		s->i--;
	}

	assert(!set_contains(s, item));
}

void
set_free(struct set *set)
{
	if (set == NULL) {
		return;
	}

	free(set->a);
	free(set);
}

size_t
set_count(const struct set *set)
{
	if (set == NULL) {
		return 0;
	}

	return set->i;
}

void
set_clear(struct set *set)
{
	if (set == NULL) {
		return;
	}

	set->i = 0;
}

void *
set_contains(const struct set *set, const void *item)
{
	size_t i;

	if (set_empty(set)) {
		return NULL;
	}

	assert(item != NULL);
	assert(set->cmp != NULL);

	i = set_search(set, item);
	if (set->cmp(item, set->a[i]) == 0) {
		return set->a[i];
	}

	return NULL;
}

int
set_cmp(const struct set *a, const struct set *b)
{
	if ((a == NULL) != (b == NULL)) {
		return (a == NULL) - (b == NULL);
	}

	if (a == NULL && b == NULL) {
		return 0;
	}

	if (a->i != b->i) {
		return a->i - b->i;
	}

	return memcmp(a->a, b->a, a->i * sizeof *a->a);

}

int
set_equal(const struct set *a, const struct set *b)
{
	if ((a == NULL) != (b == NULL)) {
		return 0;
	}

	if (a == NULL && b == NULL) {
		return 1;
	}

	if (a->i != b->i) {
		return 0;
	}

	return 0 == memcmp(a->a, b->a, a->i * sizeof *a->a);
}

int
set_empty(const struct set *set)
{
	return set == NULL || set->i == 0;
}

void *
set_first(const struct set *set, struct set_iter *it)
{
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
set_firstafter(const struct set *set, struct set_iter *it, void *item)
{
	size_t i;
	int r;

	assert(it != NULL);

	if (set_empty(set)) {
		it->set = NULL;
		return NULL;
	}

	assert(set->cmp != NULL);

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

