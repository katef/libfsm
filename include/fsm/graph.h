/* $Id$ */

#ifndef FSM_GRAPH_H
#define FSM_GRAPH_H

struct fsm;
struct fsm_state;
struct state_set;

/*
 * Reverse the given fsm. This may result in an NFA.
 *
 * Returns 1 on success, or 0 on error.
 */
int
fsm_reverse(struct fsm *fsm);

/*
 * Complement the given fsm.
 *
 * Returns 1 on success, or 0 on error.
 */
int
fsm_complement(struct fsm *fsm);

/*
 * Convert an fsm to a DFA.
 *
 * Returns false on error; see errno.
 */
int
fsm_todfa(struct fsm *fsm);

/* TODO: explain */
int
fsm_todfa_opaque(struct fsm *fsm,
	void (*carryopaque)(struct state_set *, struct fsm *, struct fsm_state *));

/*
 * Returns true if a given FSM is a DFA.
 */
int
fsm_isdfa(const struct fsm *fsm);

/*
 * Make a DFA complete, as per fsm_iscomplete.
 */
int
fsm_complete(struct fsm *fsm);

/*
 * Returns true if a given FSM is complete, or if a given state is complete.
 *
 * To be complete means that every state has an edge for all letters in the
 * alphabet (sigma). The alphabet for libfsm is all values expressible by an
 * unsigned octet.
 */
int
fsm_iscomplete(const struct fsm *fsm);
int
fsm_iscompletestate(const struct fsm *fsm, const struct fsm_state *state);


/*
 * Minimize an FSM to its canonical form.
 *
 * Returns false on error; see errno.
 */
int
fsm_minimize(struct fsm *fsm);

#endif

