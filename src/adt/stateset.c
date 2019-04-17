/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include <adt/set.h>
#include <adt/stateset.h>

struct state_set *
state_set_create(const struct fsm_alloc *a)
{
	return (struct state_set *) set_create(a, NULL);
}

struct state_set *
state_set_create_from_array(const struct fsm_alloc *a,
	struct fsm_state **states, size_t n)
{
	return (struct state_set *) set_create_from_array(a, (void **) states, n, NULL, NULL);
}

struct state_set *
state_set_create_singleton(const struct fsm_alloc *a,
	struct fsm_state *state)
{
	return (struct state_set *) set_create_singleton(a, NULL, state);
}

void
state_set_free(struct state_set *set)
{
	if (set == NULL) {
		return;
	}

	set_free((struct set *) set);
}

struct fsm_state *
state_set_add(struct state_set *set, struct fsm_state *st)
{
	assert(set != NULL);
	assert(st != NULL);

	return set_add((struct set *) set, st);
}

void
state_set_remove(struct state_set *set, const struct fsm_state *st)
{
	assert(st != NULL);

	if (set == NULL) {
		return;
	}

	set_remove((struct set *) set, st);
}

int
state_set_empty(const struct state_set *set)
{
	if (set == NULL) {
		return 1;
	}

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
	assert(st != NULL);

	if (set == NULL) {
		return NULL;
	}

	return set_contains((const struct set *) set, st);
}

size_t
state_set_count(const struct state_set *set)
{
	if (set == NULL) {
		return 0;
	}

	return set_count((const struct set *) set);
}

struct fsm_state *
state_set_first(struct state_set *set, struct state_iter *it)
{
	assert(it != NULL);

	if (set == NULL) {
		return NULL;
	}

	return set_first((struct set *) set, &it->iter);
}

struct fsm_state *
state_set_next(struct state_iter *it)
{
	assert(it != NULL);

	return set_next(&it->iter);
}

struct state_set *
state_set_copy(const struct state_set *src)
{
	return (struct state_set *)set_copy((struct set *)src);
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


