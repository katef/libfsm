#include <assert.h>
#include <stdlib.h>

#include <adt/hashset.h>
#include <adt/mappingset.h>

/* Uses a hash set and a list to hold the items that are not yet done. */
struct mapping_set {
	struct hashset *set;
};

struct mapping_set *
mapping_set_create(unsigned long (*hash)(const void *a), int (*cmp)(const void *a, const void *b))
{
	struct mapping_set *set;

	assert(hash != NULL);
	assert(cmp != NULL);

	set = malloc(sizeof *set);
	if (set == NULL) {
		return NULL;
	}

	set->set = hashset_create(hash, cmp);
	if (set->set == NULL) {
		free(set);
		return NULL;
	}

	return set;
}

void
mapping_set_free(struct mapping_set *set)
{
	if (set == NULL) {
		return;
	}

	assert(set->set != NULL);
	hashset_free(set->set);
	free(set);
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

