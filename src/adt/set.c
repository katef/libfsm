/* $Id$ */

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
};

static void
print_set(const char *prefix, const struct set *s)
{
	size_t i;

	assert(prefix != NULL);

	fprintf(stderr, "%s (%p):\n", prefix, (void *)s);

	if (set_empty(s)) {
		fprintf(stderr, "\tempty\n");
		return;
	}

	fprintf(stderr, "\t%zu of %zu\n", s->i, s->n);
	fprintf(stderr, "\t");
	for (i = 0; i < s->i; i++) {
		fprintf(stderr, "%p ", (void *) s->a[i]);
	}
	fprintf(stderr, "\n");
}

/*
 * Return where an item would be, if it were inserted
 */
static size_t
set_search(const struct set *set, const void *item)
{
	const void *p, *q;
	size_t start, end;
	size_t mid;

	assert(item != NULL);

	p = item;

	start = mid = 0;
	end = set->i;

	while (start < end) {
		mid = start + (end - start) / 2;
		q = set->a[mid];
		if (p < q) {
			end = mid;
		} else if (p > q) {
			start = mid + 1;
		} else {
			return mid;
		}
	}

	return mid;
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
	 * If the set is not initialized, go ahead and do that.
	 * Insert the new item at the front.
	 */
	if (s == NULL) {
		s = malloc(sizeof **set);
		if (s == NULL) {
			return NULL;
		}

		s->a = malloc(SET_INITIAL * sizeof *s->a);
		if (s->a == NULL) {
			return NULL;
		}

		s->a[0] = item;
		s->i = 1;
		s->n = SET_INITIAL;

		*set = s;

		assert(set_contains(*set, item));

		return item;
	}

	/*
	 * If the item already exists in the set, return success.
	 *
	 * TODO: Notify on success differently somehow when the item
	 * was already there, than if we successfully inserted
	 * a non-existing item.
	 */
	if (!set_empty(s)) {
		i = set_search(s, item);
		if (s->a[i] == item) {
			return item;
		}
	}

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

	if (item > s->a[i]) {
		i++;
	}

	memmove(&s->a[i + 1], &s->a[i], (s->i - i) * (sizeof *s->a));

	s->a[i] = item;
	s->i++;

	assert(set_contains(s, item));

	return item;
}

void
set_remove(struct set **set, void *item)
{
	struct set *s = *set;
	size_t i;

	assert(item != NULL);

	if (set_empty(s)) {
		return;
	}

	i = set_search(s, item);
	if (s->a[i] == item) {
		if (i < s->i) {
			memmove(&s->a[i], &s->a[i + 1], (s->i - i) * (sizeof *s->a));
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

int
set_contains(const struct set *set, const void *item)
{
	size_t i;

	assert(item != NULL);

	if (set_empty(set)) {
		return 0;
	}

	i = set_search(set, item);
	if (set->a[i] == item) {
		return 1;
	}

	return 0;
}

int
subsetof(const struct set *a, const struct set *b)
{
	size_t i;

	if ((a == NULL) != (b == NULL)) {
		return 0;
	}

	if (a == NULL && b == NULL) {
		return 1;
	}

	for (i = 0; i < a->i; i++) {
		if (!set_contains(b, a->a[i])) {
			return 0;
		}
	}

	return 1;
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

void
set_merge(struct set **dst, struct set *src)
{
	size_t i;

	if (set_empty(src)) {
		return;
	}

	for (i = 0; i < src->i; i++) {
		set_add(dst, src->a[i]);
	}
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

