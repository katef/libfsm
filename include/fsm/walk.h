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

/* Walk a DFA and generate inputs that reach an end state.
 * An internal buffer will be resized on demand. This generates
 * matches depth-first, bounded by max_length.
 *
 * This does not produce every possible matching input string, only
 * strings representing distinct paths through the DFA's graph. For
 * example, when given the regex /^x[aeiou]y$/ it may only generate
 * "xay", not "xey", "xiy", "xoy", or "xuy", because those are other
 * labels for the same graph edge. This function was originally added
 * to compare matching behavior against PCRE, so eliminating those
 * functionally equivalent cases makes testing dramatically faster,
 * but exploring every edge could be added later.
 *
 * Note: fsm is non-const because it calls fsm_trim on the FSM
 * internally. This records the shortest distance from each state to an
 * end state, which is used to prune branches that would not produce
 * any new matches with the remaining buffer space. This makes generation
 * dramatically faster.
 *
 * Returns 1 if iteration completes (or is halted by the callback),
 * or 0 on error.
 */
enum fsm_generate_matches_cb_res {
	/* Continue generating. */
	FSM_GENERATE_MATCHES_CB_RES_CONTINUE,
	/* Continue generating, but treat the current end state as a
	 * dead end. */
	FSM_GENERATE_MATCHES_CB_RES_PRUNE,
	/* Halt iteration entirely. Clean up and return. */
	FSM_GENERATE_MATCHES_CB_RES_HALT
};
typedef enum fsm_generate_matches_cb_res
fsm_generate_matches_cb(const struct fsm *fsm,
    size_t depth, size_t match_count, size_t steps,
    const char *input, size_t input_length,
    fsm_state_t end_state, void *opaque);
int
fsm_generate_matches(struct fsm *fsm, size_t max_length,
    fsm_generate_matches_cb *cb, void *opaque);

#endif

