#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h> /* XXX: for f_alloc */

#include <adt/hashset.h>

#include "mappingset.h"

/* Uses a hash set and a list to hold the items that are not yet done. */
struct mapping_set {
	struct hashset *set;
};

struct mapping_set *
mapping_set_create(struct fsm *fsm,
	unsigned long (*hash)(const void *a), int (*cmp)(const void *a, const void *b))
{
	static const struct mapping_set init;
	struct mapping_set *set;

	assert(hash != NULL);
	assert(cmp != NULL);

	set = f_malloc(fsm, sizeof *set); /* XXX - double check with katef */
	*set = init;
	set->set = hashset_create(hash, cmp);

	return set;
}

void
mapping_set_free(const struct fsm *fsm, struct mapping_set *set)
{
	if (set != NULL) {
		if (set->set != NULL) {
			hashset_free(set->set);
			set->set = NULL;
		}

		f_free(fsm,set);
	}
}

struct mapping *
mapping_set_add(struct mapping_set *set, struct mapping *item)
{
	assert(set != NULL);
	assert(set->set != NULL);

	if (hashset_add(set->set, item) == NULL) {
		return NULL;
	}

	return item;
}

struct mapping *
mapping_set_contains(const struct mapping_set *set, const struct mapping *item)
{
	assert(set != NULL);
	assert(set->set != NULL);

	return hashset_contains(set->set, item);
}

void
mapping_set_clear(struct mapping_set *set)
{
	assert(set != NULL);
	assert(set->set != NULL);

	hashset_clear(set->set);
}

struct mapping *
mapping_set_first(const struct mapping_set *set, struct mapping_iter *it)
{
	return hashset_first(set->set, &it->iter);
}

struct mapping *
mapping_set_next(struct mapping_iter *it)
{
	return hashset_next(&it->iter);
}

