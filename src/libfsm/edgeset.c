#include <stddef.h>
#include <stdlib.h>

#include <adt/set.h>

#include "edgeset.h"

void
edge_set_free(struct edge_set *set)
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

struct fsm_edge *
edge_set_add(struct edge_set *set, struct fsm_edge *e)
{
	return set_add(&set->set, e);
}

struct fsm_edge *
edge_set_contains(const struct edge_set *set, const struct fsm_edge *e)
{
	return set_contains(set->set, e);
}

size_t
edge_set_count(const struct edge_set *set)
{
	return set_count(set->set);
}

void
edge_set_remove(struct edge_set *set, const struct fsm_edge *e)
{
	set_remove(&set->set, (void *)e);
}

struct fsm_edge *
edge_set_first(struct edge_set *set, struct edge_iter *it)
{
	return set_first(set->set, &it->iter);
}

struct fsm_edge *
edge_set_firstafter(struct edge_set *set, struct edge_iter *it, const struct fsm_edge *e)
{
	return set_firstafter(set->set, &it->iter, (void *)e);
}

struct fsm_edge *
edge_set_next(struct edge_iter *it)
{
	return set_next(&it->iter);
}

int
edge_set_hasnext(struct edge_iter *it)
{
	return set_hasnext(&it->iter);
}

struct fsm_edge *
edge_set_only(const struct edge_set *s)
{
	return set_only(s->set);
}


