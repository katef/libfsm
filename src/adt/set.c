/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <adt/set.h>

#define SET_BASE_CAP		8

struct set {
	void **elems;
	size_t nelems;
	size_t cap;
};

static void
print_set(const char *pref, struct set *s)
{
	size_t i;

	fprintf(stderr, "%s (%p):\n", pref, (void *)s);

	if (set_empty(s)) {
		fprintf(stderr, "\tempty\n");
		return;
	}

	fprintf(stderr, "\tnelem: %zu cap: %zu\n\t", s->nelems, s->cap);
	for (i = 0; i < s->nelems; i++) {
		fprintf(stderr, "%p ", (void *)s->elems[i]);
	}
	fprintf(stderr, "\n");
}

/* Search returns where a thing would be if it was inserted
 */
static size_t
set_search(struct set *set, void *elem)
{
	size_t start, end;
	size_t mid;
	void *a = elem;

	start = mid = 0;
	end = set->nelems;

	while (start < end) {
		void *b;
		mid = start + (end - start) / 2;
		b = set->elems[mid];
		if (a < b) {
			end = mid;
		} else if (a > b) {
			start = mid + 1;
		} else {
			return mid;
		}
	}

	return mid;
}

void *
set_addelem(struct set **set, void *elem)
{
	struct set *s;
	size_t off = 0;

	assert(set != NULL);
	assert(elem != NULL);

	s = *set;

	/* If the set is not initialized, go ahead and do that. Insert the new elem at
	 * the front.
	 */
	if (s == NULL) {
		s = malloc(sizeof **set);
		if (s == NULL) {
			return NULL;
		}

		s->elems = malloc(SET_BASE_CAP * sizeof (*s->elems));

		if (s->elems == NULL) {
			return NULL;
		}

		s->elems[0] = elem;
		s->nelems = 1;
		s->cap = SET_BASE_CAP;

		*set = s;

		assert(set_contains(*set, elem));
		return elem;
	}

	/* If the elem already exists in the set, return success.
	 * XXX: Notify on success differently somehow when elem was already there than
	 * if we successfully inserted a non-existing elem.
	 */
	if (!set_empty(s)) {
		off = set_search(s, elem);
		if (s->elems[off] == elem) {
			return elem;
		}
	}

	/* We're at capacity. Get more */
	if (s->cap == s->nelems) {
		void **new;

		new = realloc(s->elems, sizeof (*s->elems) * (s->cap * 2));
		if (new == NULL) {
			return NULL;
		}

		s->elems = new;
		s->cap *= 2;
	}

	if (elem > s->elems[off]) {
		off++;
	}

	memmove(&s->elems[off + 1], &s->elems[off],
	    (s->nelems - off) * sizeof (*s->elems));

	s->elems[off] = elem;
	s->nelems++;

	assert(set_contains(s, elem));

	return elem;
}

void
set_remove(struct set **set, void *elem)
{
	struct set *s = *set;
	size_t off;

	if (set_empty(s)) {
		return;
	}

	assert(elem != NULL);

	off = set_search(s, elem);
	if (s->elems[off] == elem) {
		if (off < s->nelems) {
			memmove(&s->elems[off], &s->elems[off + 1],
			    (s->nelems - off) * sizeof (*s->elems));
		}

		s->nelems--;
	}

	assert(!set_contains(s, elem));
}

void
set_free(struct set *set)
{

	if (set == NULL) {
		return;
	}

	free(set->elems);
	free(set);
}

int
set_contains(const struct set *set, const void *elem)
{
	size_t off;

	if (set_empty(set)) {
		return 0;
	}

	off = set_search(set, elem);
	if (set->elems[off] == elem) {
		return 1;
	}

	return 0;
}

int
subsetof(const struct set *a, const struct set *b)
{
	size_t off;

	if ((!a && b) || (!b && a)) {
		return 0;
	} else if (!a && !b) {
		return 1;
	}

	for (off = 0; off < a->nelems; off++) {
		if (!set_contains(b, a->elems[off])) {
			return 0;
		}
	}

	return 1;
}

int
set_equal(const struct set *a, const struct set *b)
{
	return subsetof(a, b) && subsetof(b, a);
}

void
set_merge(struct set **dst, struct set *src)
{
	size_t i;

	if (set_empty(src)) {
		return;
	}

	for (i = 0; i < src->nelems; i++) {
		set_addelem(dst, src->elems[i]);
	}
}

int
set_empty(struct set *set)
{

	return set == NULL || set->nelems == 0;
}

void *
set_first(struct set *set, struct set_iter *i)
{

	assert(i != NULL);

	if (set_empty(set)) {
		i->set = NULL;
		return NULL;
	}

	i->cursor = 0;
	i->set = set;

	return i->set->elems[i->cursor];
}

void *
set_next(struct set_iter *i)
{

	assert(i != NULL);

	i->cursor++;
	if (i->cursor >= i->set->nelems) {
		return NULL;
	}

	return i->set->elems[i->cursor];
}

int
set_hasnext(struct set_iter *i)
{

	assert(i != NULL);

	return i->set && i->cursor + 1 < i->set->nelems;
}
