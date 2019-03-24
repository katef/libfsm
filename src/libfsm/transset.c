#include <assert.h>
#include <stddef.h>

#include <fsm/fsm.h> /* XXX: for f_alloc */

#include <adt/set.h>
#include <adt/hashset.h>

#include "transset.h"

struct trans_set {
	struct set *set;
};

struct trans_set *
trans_set_create(struct fsm *fsm,
	int (*cmp)(const void *a, const void *b))
{
	static const struct trans_set init;
	struct trans_set *set;

	assert(cmp != NULL);

	set = f_malloc(fsm,sizeof *set); /* XXX - double check with katef */
	*set = init;
	set->set = set_create(cmp);
	return set;
}

void
trans_set_free(struct fsm *fsm, struct trans_set *set)
{
	if (set != NULL) {
		if (set->set != NULL) {
			set_free(set->set);
			set->set = NULL;
		}
		f_free(fsm,set);
	}
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

