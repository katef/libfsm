/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <adt/hashset.h>
#include <adt/statehashset.h>

struct state_hashset *
state_hashset_create(const struct fsm_alloc *a,
	unsigned long (*hash)(const void *a),
	int (*cmp)(const void *a, const void *b))
{
	assert(hash != NULL);
	assert(cmp != NULL);

	return (struct state_hashset *) hashset_create(a,
		hash, cmp);
}

void
state_hashset_free(struct state_hashset *set)
{
	hashset_free((struct hashset *) set);
}

struct fsm_state *
state_hashset_add(struct state_hashset *set, struct fsm_state *item)
{
	assert(set != NULL);

	return hashset_add((struct hashset *) set, item);
}

struct fsm_state *
state_hashset_contains(const struct state_hashset *set, const struct fsm_state *item)
{
	assert(set != NULL);

	return hashset_contains((const struct hashset *) set, item);
}

