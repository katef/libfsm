/* $Id$ */

#ifndef FSM_GRAPH_H
#define FSM_GRAPH_H

struct fsm;

/*
 * Reverse the given fsm. This may result in an NFA.
 *
 * Returns fsm on success, or NULL on error.
 */
struct fsm *
fsm_reverse(struct fsm *fsm);

/*
 * Convert an fsm to a DFA.
 *
 * Returns fsm on success, or NULL on error.
 */
struct fsm *
fsm_todfa(struct fsm *fsm);

#endif

