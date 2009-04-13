/* $Id$ */

#ifndef FSM_GRAPH_H
#define FSM_GRAPH_H

struct fsm;

/*
 * Reverse the given fsm. This may result in an NFA.
 *
 * Returns 1 on success, or 0 on error.
 */
int
fsm_reverse(struct fsm *fsm);

/*
 * Convert an fsm to a DFA.
 *
 * Returns 1 on success, 0 on error, or -1 if conversion to DFA fails due to an
 * opaque constraint violation. If no non-NULL opaque values are present, then
 * -1 can never be returned. See fsm_setopaque() for details.
 */
int
fsm_todfa(struct fsm *fsm);

/*
 * Returns true if a given FSM is a DFA.
 */
int
fsm_isdfa(const struct fsm *fsm);

/*
 * Minimize an FSM to its canonical form.
 *
 * Returns 1 on success, 0 on error, or -1 if minimization fails due to an
 * opaque constraint violation. If no non-NULL opaque values are present, then
 * -1 can never be returned. See fsm_setopaque() for details.
 */
int
fsm_minimize(struct fsm *fsm);

#endif

