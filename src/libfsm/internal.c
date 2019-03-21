#include <stddef.h>
#include <stdlib.h>

#include "internal.h"

/* edge set */

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


/* state set */

struct state_set *
state_set_create(void)
{
	static const struct state_set init;
	struct state_set *set;

	set = malloc(sizeof *set);
	if (!set) {
		return NULL;
	}

	*set = init;
	set->set = NULL;

	return set;
}

void
state_set_free(struct state_set *set)
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

struct fsm_state *
state_set_add(struct state_set *set, struct fsm_state *st)
{
	return set_add(&set->set, st);
}

void
state_set_remove(struct state_set *set, const struct fsm_state *st)
{
	set_remove(&set->set, (void *)st);
}

int
state_set_empty(const struct state_set *s)
{
	return set_empty(s->set);
}

struct fsm_state *
state_set_only(const struct state_set *s)
{
	return set_only(s->set);
}

struct fsm_state *
state_set_contains(const struct state_set *set, const struct fsm_state *st)
{
	return set_contains(set->set, st);
}

size_t
state_set_count(const struct state_set *set)
{
	return set_count(set->set);
}

struct fsm_state *
state_set_first(struct state_set *set, struct state_iter *it)
{
	return set_first(set->set, &it->iter);
}

struct fsm_state *
state_set_next(struct state_iter *it)
{
	return set_next(&it->iter);
}

int
state_set_hasnext(struct state_iter *it)
{
	return set_hasnext(&it->iter);
}

const struct fsm_state **
state_set_array(const struct state_set *set)
{
	return (const struct fsm_state **)set_array(set->set);
}

int
state_set_cmp(const struct state_set *a, const struct state_set *b)
{
	return set_cmp(a->set, b->set);
}


