/* $Id$ */

#ifndef FSM_GRAPH_H
#define FSM_GRAPH_H

struct fsm;

/*
 * Return a copy of the given fsm, reversed. This may be an NFA.
 */
struct fsm *
fsm_reverse(const struct fsm *fsm);

#endif

