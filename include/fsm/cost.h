/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_COST_H
#define FSM_COST_H

struct fsm_state;

#define FSM_COST_INFINITY UINT_MAX

unsigned
fsm_cost_legible(const struct fsm_state *from, const struct fsm_state *to, char c);

#endif

