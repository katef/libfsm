/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_PRED_H
#define FSM_PRED_H

/*
 * Predicates.
 */

struct fsm;
struct fsm_state;

int
fsm_isany(const struct fsm *fsm, fsm_state_t state);

int
fsm_isend(const struct fsm *fsm, fsm_state_t state);

int
fsm_isdfa(const struct fsm *fsm, fsm_state_t state);

/*
 * To be complete means that a state has an edge for all letters in the
 * alphabet (sigma). The alphabet for libfsm is all values expressible by an
 * unsigned octet.
 */
int
fsm_iscomplete(const struct fsm *fsm, fsm_state_t state);

int
fsm_hasincoming(const struct fsm *fsm, fsm_state_t state);

int
fsm_hasoutgoing(const struct fsm *fsm, fsm_state_t state);

/*
 * One or more epsilons.
 */
int
fsm_hasepsilons(const struct fsm *fsm, fsm_state_t state);

/*
 * True iff there are outgoing edges and all are epsilons.
 */
int
fsm_epsilonsonly(const struct fsm *fsm, fsm_state_t state);

/*
 * True if there is more than one edge labelled with the same symbol
 * reachable from a state. This includes edges from states reached
 * by epsilon transitions.
 */
int
fsm_hasnondeterminism(const struct fsm *fsm, fsm_state_t state);

#endif

