/* $Id$ */

#ifndef FSM_GRAPH_H
#define FSM_GRAPH_H

struct fsm;
struct fsm_state;
struct state_set;

/*
 * Make the given fsm case-insensitive. This may result in an NFA.
 *
 * Returns 1 on success, or 0 on error.
 */
int
fsm_desensitise(struct fsm *fsm);

/*
 * Reverse the given fsm. This may result in an NFA.
 *
 * Returns 1 on success, or 0 on error.
 */
int
fsm_reverse(struct fsm *fsm);

/* TODO: explain */
int
fsm_reverse_opaque(struct fsm *fsm,
	void (*carryopaque)(struct state_set *, struct fsm *, struct fsm_state *));

/*
 * Convert an fsm to a DFA.
 *
 * Returns false on error; see errno.
 */
int
fsm_determinise(struct fsm *fsm);

/* TODO: explain */
int
fsm_determinise_opaque(struct fsm *fsm,
	void (*carryopaque)(struct state_set *, struct fsm *, struct fsm_state *));

/*
 * Make a DFA complete, as per fsm_iscomplete.
 */
int
fsm_complete(struct fsm *fsm,
	int (*predicate)(const struct fsm *, const struct fsm_state *));

/*
 * Minimize an FSM to its canonical form.
 *
 * Returns false on error; see errno.
 */
int
fsm_minimise(struct fsm *fsm);

/*
 * Concatenate b after a. This is not commutative.
 */
struct fsm *
fsm_concat(struct fsm *a, struct fsm *b);

/*
 * Subtract b from a. This is not commutative.
 */
struct fsm *
fsm_subtract(struct fsm *a, struct fsm *b);

#endif

