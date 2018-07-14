/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_WALK_H
#define FSM_WALK_H

struct fsm;
struct fsm_state;

/*
 * Count states for which the given predicate is true.
 */
unsigned
fsm_count(const struct fsm *fsm,
	int (*predicate)(const struct fsm *, const struct fsm_state *));

/*
 * Return true if the given predicate is true for any state.
 */
int
fsm_has(const struct fsm *fsm,
	int (*predicate)(const struct fsm *, const struct fsm_state *));

/*
 * Run a given predicate against all states in an FSM.
 *
 * Returns true if the predicate is true for every state and false otherwise.
 */
int
fsm_all(const struct fsm *fsm,
	int (*predicate)(const struct fsm *, const struct fsm_state *));

/*
 * Assert that the given predicate holds for all states reachable
 * (by epsilon transitions or otherwise) from the given state.
 *
 * Returns 1 if the predicate holds for all/any states reached, respectively.
 * Returns 0 otherwise.
 *
 * Returns -1 on error.
 */
int
fsm_reachableall(const struct fsm *fsm, const struct fsm_state *state,
	int (*predicate)(const struct fsm *, const struct fsm_state *));
int
fsm_reachableany(const struct fsm *fsm, const struct fsm_state *state,
	int (*predicate)(const struct fsm *, const struct fsm_state *));

/* Allows iterating through the states of the graph with a callback
 * function.  Takes an opaque pointer that the callback can use for its
 * own purposes.
 */
int
fsm_walk_states(const struct fsm *fsm, void *opaque,
	int (*callback)(const struct fsm *, const struct fsm_state *, void *));

/* Allows iterating through the states of the graph with a callback
 * function.  Takes an opaque pointer that the callback can use for its
 * own purposes.
 */
int
fsm_walk_edges(const struct fsm *fsm, void *opaque,
	int (*callback)(const struct fsm *, const struct fsm_state *, unsigned int, const struct fsm_state *, void *));

#endif

