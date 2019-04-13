/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>

#include <adt/set.h>
#include <adt/stateset.h>

struct state_set *
state_set_create(void)
{
	return (struct state_set *) set_create(NULL);
}

void
state_set_free(struct state_set *set)
{
	assert(set != NULL);

	set_free((struct set *) set);
}

struct fsm_state *
state_set_add(struct state_set *set, struct fsm_state *st)
{
	assert(set != NULL);

	return set_add((struct set *) set, st);
}

void
state_set_remove(struct state_set *set, const struct fsm_state *st)
{
	assert(set != NULL);

	set_remove((struct set *) set, st);
}

int
state_set_empty(const struct state_set *set)
{
	assert(set != NULL);

	return set_empty((const struct set *) set);
}

struct fsm_state *
state_set_only(const struct state_set *set)
{
	assert(set != NULL);

	return set_only((const struct set *) set);
}

struct fsm_state *
state_set_contains(const struct state_set *set, const struct fsm_state *st)
{
	assert(set != NULL);

	return set_contains((const struct set *) set, st);
}

size_t
state_set_count(const struct state_set *set)
{
	assert(set != NULL);

	return set_count((const struct set *) set);
}

struct fsm_state *
state_set_first(struct state_set *set, struct state_iter *it)
{
	assert(set != NULL);

	return set_first((struct set *) set, &it->iter);
}

struct fsm_state *
state_set_next(struct state_iter *it)
{
	assert(it != NULL);

	return set_next(&it->iter);
}

int
state_set_hasnext(struct state_iter *it)
{
	assert(it != NULL);

	return set_hasnext(&it->iter);
}

const struct fsm_state **
state_set_array(const struct state_set *set)
{
	assert(set != NULL);

	return (const struct fsm_state **) set_array((const struct set *) set);
}

int
state_set_cmp(const struct state_set *a, const struct state_set *b)
{
	assert(a != NULL);
	assert(b != NULL);

	return set_cmp((const struct set *) a, (const struct set *) b);
}


