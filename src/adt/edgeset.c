/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include "libfsm/internal.h" /* XXX: for allocating struct fsm_edge */

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

typedef struct fsm_edge item_t;

#include "set.inc"

struct edge_set *
edge_set_create(const struct fsm_alloc *a,
	int (*cmp)(const void *a, const void *b))
{
	assert(cmp != NULL);

	return (struct edge_set *) set_create(a, cmp);
}

void
edge_set_free(struct edge_set *set)
{
	if (set == NULL) {
		return;
	}

	set_free((struct set *) set);
}

struct fsm_edge *
edge_set_add(struct edge_set *set, struct fsm_edge *e)
{
	assert(set != NULL);
	assert(e != NULL);

	return set_add((struct set *) set, e);
}

struct fsm_edge *
edge_set_contains(const struct edge_set *set, const struct fsm_edge *e)
{
	assert(set != NULL);
	assert(e != NULL);

	return set_contains((const struct set *) set, e);
}

size_t
edge_set_count(const struct edge_set *set)
{
	if (set == NULL) {
		return 0;
	}

	return set_count((const struct set *) set);
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

	set_remove((struct set *) set, e);
}

struct fsm_edge *
edge_set_first(struct edge_set *set, struct edge_iter *it)
{
	assert(it != NULL);

	if (set == NULL) {
		return NULL;
	}

	return set_first((struct set *) set, &it->iter);
}

struct fsm_edge *
edge_set_firstafter(const struct edge_set *set, struct edge_iter *it, const struct fsm_edge *e)
{
	assert(set != NULL);
	assert(it != NULL);
	assert(e != NULL);

	return set_firstafter((const struct set *) set, &it->iter, e);
}

struct fsm_edge *
edge_set_next(struct edge_iter *it)
{
	assert(it != NULL);

	return set_next(&it->iter);
}

int
edge_set_hasnext(struct edge_iter *it)
{
	assert(it != NULL);

	return set_hasnext(&it->iter);
}

int
edge_set_empty(const struct edge_set *set)
{
	if (set == NULL) {
		return 1;
	}

	return set_empty((const struct set *) set);
}

struct fsm_edge *
edge_set_only(const struct edge_set *set)
{
	assert(set != NULL);

	return set_only((const struct set *) set);
}

