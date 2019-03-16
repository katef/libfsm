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

typedef int (fsm_pred)(const struct fsm *fsm, const struct fsm_state *state);

fsm_pred fsm_isany;
fsm_pred fsm_isend;
fsm_pred fsm_isdfa;

/*
 * To be complete means that a state has an edge for all letters in the
 * alphabet (sigma). The alphabet for libfsm is all values expressible by an
 * unsigned octet.
 */
fsm_pred fsm_iscomplete;

fsm_pred fsm_hasincoming;
fsm_pred fsm_hasoutgoing;

/*
 * True iff there are outgoing edges and all are epsilons.
 */
fsm_pred fsm_epsilonsonly;

#endif

