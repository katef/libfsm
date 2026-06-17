/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_WALK_H
#define FSM_WALK_H

#include <adt/bitmap.h>

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
 * If randomized is zero then it will generate the first label in the
 * label set, otherwise a label from the set will be chosen using rand()
 * (favoring printable characters). The caller can use srand()
 * beforehand to set a PRNG seed.
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
fsm_generate_matches(struct fsm *fsm, size_t max_length, int randomized,
    fsm_generate_matches_cb *cb, void *opaque);

/* Callback provided for the most basic use case for
 * fsm_generate_matches: exhaustively generate input up to
 * max_length and print each to stdout. */
fsm_generate_matches_cb fsm_generate_cb_printf;

/* Same as fsm_generate_cb_printf, but call c_escputc_str
 * internally to escape characters.
 *
 * Note: This MUST be called with opaque set to a `const struct
 * fsm_options *`, because c_escputc_str will use that to decide whether
 * to escape all characters or just nonprintable ones. */
fsm_generate_matches_cb fsm_generate_cb_printf_escaped;

/* Walk a DFA and detect which characters MUST appear in the input for a
 * match to be possible. For example, if input for the DFA corresponding
 * to /^(abc|dbe)$/ does not contain 'b' at all, there's no way it can
 * ever match, so executing the regex is unnecessary. This does not detect
 * which characters must appear before/after others or how many times, just
 * which must be present.
 *
 * The input must be a DFA. When run with EXPENSIVE_CHECKS this will
 * check and return ERROR_MISUSE if it is not, otherwise this is an
 * unchecked error.
 *
 * The character map will be cleared before populating. If *count is
 * non-NULL it will be updated with how many required characters were
 * found.
 *
 * There is an optional step_limit -- if this is reached, then it will
 * return FSM_DETECT_REQUIRED_CHARACTERS_STEP_LIMIT_REACHED and a
 * cleared bitmap, because any partial information could still have been
 * contradicted later. If the step_limit is 0 it will be ignored. */
enum fsm_detect_required_characters_res {
	FSM_DETECT_REQUIRED_CHARACTERS_WRITTEN,
	FSM_DETECT_REQUIRED_CHARACTERS_STEP_LIMIT_REACHED,
	FSM_DETECT_REQUIRED_CHARACTERS_ERROR_MISUSE = -1,
	FSM_DETECT_REQUIRED_CHARACTERS_ERROR_ALLOC = -2,
};
enum fsm_detect_required_characters_res
fsm_detect_required_characters(const struct fsm *dfa, size_t step_limit,
    uint64_t charmap[4], size_t *count);

#endif

