/* $Id$ */

#ifndef SET_H
#define SET_H

struct fsm_state;

struct state_set {
	struct fsm_state *state;
	struct state_set *next;
};

struct state_set *
set_addstate(struct state_set **head, struct fsm_state *state);

void
set_remove(struct state_set **head, struct fsm_state *state);

void
set_replace(struct state_set *set,
	struct fsm_state *old, struct fsm_state *new);

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

#endif

