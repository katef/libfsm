#include <assert.h>
#include <stdlib.h>

#include <adt/set.h>
#include <adt/hashset.h>

#include "transset.h"

struct trans_set {
	struct set *set;
};

struct trans_set *
trans_set_create(int (*cmp)(const void *a, const void *b))
{
	struct trans_set *set;

	assert(cmp != NULL);

	set = malloc(sizeof *set);
	if (set == NULL) {
		return NULL;
	}

	set->set = set_create(cmp);
	if (set->set == NULL) {
		free(set);
		return NULL;
	}

	return set;
}

void
trans_set_free(struct trans_set *set)
{
	if (set == NULL) {
		return;
	}

	set_free(set->set);
	free(set);
}

struct trans *
trans_set_add(struct trans_set *set, struct trans *item)
{
	return set_add(&set->set, item);
}

struct trans *
trans_set_contains(const struct trans_set *set, const struct trans *item)
{
	return set_contains(set->set, item);
}

void
trans_set_clear(struct trans_set *set)
{
	set_clear(set->set);
}

struct trans *
trans_set_first(const struct trans_set *set, struct trans_iter *it)
{
	return set_first(set->set, &it->iter);
}

struct trans *
trans_set_next(struct trans_iter *it)
{
	return set_next(&it->iter);
}

