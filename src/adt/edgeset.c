/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <adt/set.h>
#include <adt/edgeset.h>

struct edge_set {
	struct set *set;
};

struct edge_set *
edge_set_create(int (*cmp)(const void *a, const void *b))
{
	struct edge_set *set;

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
edge_set_free(struct edge_set *set)
{
	if (set == NULL) {
		return;
	}

	assert(set->set != NULL);
	set_free(set->set);

	free(set);
}

struct fsm_edge *
edge_set_add(struct edge_set *set, struct fsm_edge *e)
{
	return set_add(set->set, e);
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
	set_remove(set->set, (void *)e);
}

struct fsm_edge *
edge_set_first(struct edge_set *set, struct edge_iter *it)
{
	return set_first(set->set, &it->iter);
}

struct fsm_edge *
edge_set_firstafter(const struct edge_set *set, struct edge_iter *it, const struct fsm_edge *e)
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


