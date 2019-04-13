/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <adt/set.h>
#include <adt/edgeset.h>

struct edge_set *
edge_set_create(int (*cmp)(const void *a, const void *b))
{
	return (struct edge_set *) set_create(cmp);
}

void
edge_set_free(struct edge_set *set)
{
	assert(set != NULL);

	set_free((struct set *) set);
}

struct fsm_edge *
edge_set_add(struct edge_set *set, struct fsm_edge *e)
{
	assert(set != NULL);

	return set_add((struct set *) set, e);
}

struct fsm_edge *
edge_set_contains(const struct edge_set *set, const struct fsm_edge *e)
{
	assert(set != NULL);

	return set_contains((const struct set *) set, e);
}

size_t
edge_set_count(const struct edge_set *set)
{
	assert(set != NULL);

	return set_count((const struct set *) set);
}

void
edge_set_remove(struct edge_set *set, const struct fsm_edge *e)
{
	assert(set != NULL);

	set_remove((struct set *) set, e);
}

struct fsm_edge *
edge_set_first(struct edge_set *set, struct edge_iter *it)
{
	assert(set != NULL);

	return set_first((struct set *) set, &it->iter);
}

struct fsm_edge *
edge_set_firstafter(const struct edge_set *set, struct edge_iter *it, const struct fsm_edge *e)
{
	assert(set != NULL);

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

struct fsm_edge *
edge_set_only(const struct edge_set *set)
{
	assert(set != NULL);

	return set_only((const struct set *) set);
}

