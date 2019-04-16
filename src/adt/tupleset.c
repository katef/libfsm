/*
 * Copyright 2017 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <adt/set.h>
#include <adt/tupleset.h>

struct tuple_set *
tuple_set_create(const struct fsm_alloc *a,
	int (*cmp)(const void *, const void *))
{
	assert(cmp != NULL);

	return (struct tuple_set *) set_create(a, cmp);
}

void
tuple_set_free(struct tuple_set *set)
{
	assert(set != NULL);

	set_free((struct set *) set);
}

struct fsm_walk2_tuple *
tuple_set_contains(const struct tuple_set *set, const struct fsm_walk2_tuple *item)
{
	assert(set != NULL);

	return set_contains((struct set *) set, item);
}

struct fsm_walk2_tuple *
tuple_set_add(struct tuple_set *set, struct fsm_walk2_tuple *item)
{
	assert(set != NULL);

	return set_add((struct set *) set, item);
}

