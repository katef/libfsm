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
#include <adt/hashset.h>

#define SET_INITIAL 8

struct set {
	const struct fsm_alloc *alloc;
	item_t **a;
	size_t i;
	size_t n;
	int (*cmp)(const void *, const void *);
};

static int
set_empty(const struct set *set)
{
	assert(set != NULL);

	return set->i == 0;
}

/*
 * Return where an item would be, if it were inserted
 */
static size_t
set_search(const struct set *set, const item_t *item)
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

static item_t *
set_contains(const struct set *set, const item_t *item)
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

static struct set *
set_create(const struct fsm_alloc *a,
	int (*cmp)(const void *a, const void *b))
{
	struct set *set;

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

static item_t *
set_add(struct set *set, item_t *item)
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
			item_t **new;

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

		if (i <= set->i) {
			memmove(&set->a[i + 1], &set->a[i], (set->i - i) * (sizeof *set->a));
		}

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
set_free(struct set *set)
{
	assert(set != NULL);
	assert(set->a != NULL);

	free(set->a);
	free(set);
}

