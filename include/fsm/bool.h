/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_BOOL_H
#define FSM_BOOL_H

/*
 * Boolean operators.
 *
 * The resulting FSM may be an NFA.
 *
 * Binary operators are commutative. They are also destructive,
 * and free their operands as if by fsm_free().
 */

struct fsm;

/* When combining two FSMs, it can sometimes be useful to know the
 * offsets to their original start states and captures.
 * If passed in, this struct will be updated with those details. */
struct fsm_combine_info {
	fsm_state_t base_a;
	fsm_state_t base_b;
	unsigned capture_base_a;
	unsigned capture_base_b;
};

struct fsm_combined_base_pair {
	fsm_state_t state;
	unsigned capture;
};

int
fsm_complement(struct fsm *fsm);

struct fsm *
fsm_union(struct fsm *a, struct fsm *b,
	struct fsm_combine_info *combine_info);

/* Union an array of FSMs, keeping track of the offsets to their
 * starting states and (if used) capture bases.
 *
 * bases[] is expected to have the same count as fsms[], and
 * will be initialized by the function. May be NULL.
 *
 * On success, returns the unioned fsm and updates bases[].
 * On failure, returns NULL; all fsms are freed. */
struct fsm *
fsm_union_array(size_t fsm_count,
    struct fsm **fsms, struct fsm_combined_base_pair *bases);

struct fsm *
fsm_intersect(struct fsm *a, struct fsm *b);

/*
 * A convenience to intersect against a character set, rather than
 * a pre-existing FSM. Unlike fsm_intersect(), the FSM is required
 * to be a DFA.
 */
struct fsm *
fsm_intersect_charset(struct fsm *a, size_t n, const char *charset);

/*
 * Subtract b from a. This is not commutative.
 */
struct fsm *
fsm_subtract(struct fsm *a, struct fsm *b);

#endif

