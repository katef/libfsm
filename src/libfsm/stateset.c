#include <stddef.h>
#include <stdlib.h>

#include <adt/set.h>

#include "stateset.h"

struct state_set *
state_set_create(void)
{
	struct state_set *set;

	set = malloc(sizeof *set);
	if (set == NULL) {
		return NULL;
	}

	set->set = NULL;

	return set;
}

void
state_set_free(struct state_set *set)
{
	if (set == NULL) {
		return;
	}

	set_free(set->set);
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


