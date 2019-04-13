/*
 * Copyright 2017 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include <adt/set.h>
#include <adt/tupleset.h>

struct tuple_set {
	struct set *set;
};

struct tuple_set *
tuple_set_create(int (*cmp)(const void *, const void *))
{
	static const struct tuple_set init;
	struct tuple_set *set;

	assert(cmp != NULL);

	set = malloc(sizeof *set);
	if (!set) {
		return NULL;
	}
	*set = init;
	set->set = set_create(cmp);
	if (set->set == NULL) {
		free(set);
		return NULL;
	}

	return set;
}

void
tuple_set_free(struct tuple_set *set)
{
	if (set == NULL) {
		return;
	}

	if (set->set != NULL) {
		set_free(set->set);
		set->set = NULL;
	}

	free(set);
}

struct fsm_walk2_tuple *
tuple_set_contains(struct tuple_set *set, const struct fsm_walk2_tuple *item)
{
	return set_contains(set->set, (const void *)item);
}

struct fsm_walk2_tuple *
tuple_set_add(struct tuple_set *set, const struct fsm_walk2_tuple *item)
{
	return set_add(&set->set, (void *)item);
}

