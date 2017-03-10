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
 * Returns 1 if the predicate holds for all states reached, 0 otherwise.
 *
 * Returns -1 on error.
 */
int
fsm_reachable(struct fsm *fsm, struct fsm_state *state,
	int (*predicate)(const struct fsm *, const struct fsm_state *));

#endif

