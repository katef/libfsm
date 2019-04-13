/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <adt/set.h>
#include <adt/hashset.h>
#include <adt/transset.h>

struct trans_set *
trans_set_create(int (*cmp)(const void *a, const void *b))
{
	assert(cmp != NULL);

	return (struct trans_set *) set_create(cmp);
}

void
trans_set_free(struct trans_set *set)
{
	assert(set != NULL);

	set_free((struct set *) set);
}

struct trans *
trans_set_add(struct trans_set *set, struct trans *item)
{
	assert(set != NULL);

	return set_add((struct set *) set, item);
}

struct trans *
trans_set_contains(const struct trans_set *set, const struct trans *item)
{
	assert(set != NULL);

	return set_contains((const struct set *) set, item);
}

void
trans_set_clear(struct trans_set *set)
{
	assert(set != NULL);

	set_clear((struct set *) set);
}

struct trans *
trans_set_first(const struct trans_set *set, struct trans_iter *it)
{
	assert(set != NULL);

	return set_first((const struct set *) set, &it->iter);
}

struct trans *
trans_set_next(struct trans_iter *it)
{
	assert(it != NULL);

	return set_next(&it->iter);
}

