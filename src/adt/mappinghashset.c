/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <adt/hashset.h>
#include <adt/mappinghashset.h>

struct mapping_hashset *
mapping_hashset_create(const struct fsm_alloc *a,
	unsigned long (*hash)(const void *a),
	int (*cmp)(const void *a, const void *b))
{
	assert(hash != NULL);
	assert(cmp != NULL);

	return (struct mapping_hashset *) hashset_create(a, hash, cmp);
}

void
mapping_hashset_free(struct mapping_hashset *set)
{
	hashset_free((struct hashset *) set);
}

struct mapping *
mapping_hashset_add(struct mapping_hashset *hashset, struct mapping *item)
{
	assert(hashset != NULL);

	return hashset_add((struct hashset *) hashset, item);
}

struct mapping *
mapping_hashset_contains(const struct mapping_hashset *hashset, const struct mapping *item)
{
	assert(hashset != NULL);

	return hashset_contains((const struct hashset *) hashset, item);
}

void
mapping_hashset_clear(struct mapping_hashset *hashset)
{
	assert(hashset != NULL);

	hashset_clear((struct hashset *) hashset);
}

struct mapping *
mapping_hashset_first(const struct mapping_hashset *hashset, struct mapping_hashset_iter *it)
{
	return hashset_first((const struct hashset *) hashset, &it->iter);
}

struct mapping *
mapping_hashset_next(struct mapping_hashset_iter *it)
{
	return hashset_next(&it->iter);
}

