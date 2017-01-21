/* $Id$ */

#ifndef SET_H
#define SET_H

struct fsm_state;
struct state_set;

struct set_iter {
	size_t cursor;
	struct state_set *set;
};

struct fsm_state *
set_addstate(struct state_set **set, struct fsm_state *state);

void
set_remove(struct state_set **set, struct fsm_state *state);

void
set_free(struct state_set *set);

/*
 * Find if a state is in a set.
 */
int
set_contains(const struct state_set *set, const struct fsm_state *state);

/*
 * Find if a is a subset of b
 */
int
subsetof(const struct state_set *a, const struct state_set *b);

/*
 * Compare two sets of states for equality.
 */
int
set_equal(const struct state_set *a, const struct state_set *b);

void
set_merge(struct state_set **dst, struct state_set *src);

int
set_empty(struct state_set *set);

struct fsm_state *
set_first(struct state_set *set, struct set_iter *i);

struct fsm_state *
set_next(struct set_iter *i);

int
set_hasnext(struct set_iter *i);

#endif

