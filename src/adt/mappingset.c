/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <adt/hashset.h>
#include <adt/mappingset.h>

struct mapping_set *
mapping_set_create(const struct fsm_alloc *a,
	unsigned long (*hash)(const void *a),
	int (*cmp)(const void *a, const void *b))
{
	assert(hash != NULL);
	assert(cmp != NULL);

	return (struct mapping_set *) hashset_create(a, hash, cmp);
}

void
mapping_set_free(struct mapping_set *set)
{
	hashset_free((struct hashset *) set);
}

struct mapping *
mapping_set_add(struct mapping_set *set, struct mapping *item)
{
	assert(set != NULL);

	return hashset_add((struct hashset *) set, item);
}

struct mapping *
mapping_set_contains(const struct mapping_set *set, const struct mapping *item)
{
	assert(set != NULL);

	return hashset_contains((const struct hashset *) set, item);
}

void
mapping_set_clear(struct mapping_set *set)
{
	assert(set != NULL);

	hashset_clear((struct hashset *) set);
}

struct mapping *
mapping_set_first(const struct mapping_set *set, struct mapping_iter *it)
{
	return hashset_first((const struct hashset *) set, &it->iter);
}

struct mapping *
mapping_set_next(struct mapping_iter *it)
{
	return hashset_next(&it->iter);
}

