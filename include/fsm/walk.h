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
	int (*predicate)(const struct fsm *, fsm_state_t));

/*
 * Return true if the given predicate is true for any state.
 */
int
fsm_has(const struct fsm *fsm,
	int (*predicate)(const struct fsm *, fsm_state_t));

/*
 * Run a given predicate against all states in an FSM.
 *
 * Returns true if the predicate is true for every state and false otherwise.
 */
int
fsm_all(const struct fsm *fsm,
	int (*predicate)(const struct fsm *, fsm_state_t));

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
fsm_reachableall(const struct fsm *fsm, fsm_state_t state,
	int (*predicate)(const struct fsm *, fsm_state_t));
int
fsm_reachableany(const struct fsm *fsm, fsm_state_t state,
	int (*predicate)(const struct fsm *, fsm_state_t));

/*
 * Iterate through the states of an FSM with a callback function.
 *
 * Takes an opaque pointer that the callback can use for its own purposes.
 *
 * If the callback returns 0, will stop iterating and return 0.
 * Otherwise will call the callback for each state and return 1.
 */
int
fsm_walk_states(const struct fsm *fsm, void *opaque,
	int (*callback)(const struct fsm *, fsm_state_t, void *));

/*
 * Iterate through the states of an FSM with callback functions for
 * labelled and epsilon transitions respectively.
 *
 * Takes an opaque pointer that the callback can use for its own purposes.
 *
 * If these callbacks returns 0, will stop iterating and return 0.
 * Otherwise will call the callback for each state and return 1.
 */
int
fsm_walk_edges(const struct fsm *fsm, void *opaque,
	int (*callback_literal)(const struct fsm *, fsm_state_t, fsm_state_t, char c, void *),
	int (*callback_epsilon)(const struct fsm *, fsm_state_t, fsm_state_t, void *));

#endif

