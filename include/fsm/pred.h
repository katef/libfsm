/* $Id$ */

#ifndef FSM_PRED_H
#define FSM_PRED_H

/*
 * Predicates.
 */

struct fsm;
struct fsm_state;

int
fsm_isend(const struct fsm *fsm, const struct fsm_state *state);

int
fsm_isdfastate(const struct fsm *fsm, const struct fsm_state *state);

#endif

