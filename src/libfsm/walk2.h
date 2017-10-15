/*
 * Copyright 2017 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_WALK2_H
#define FSM_WALK2_H

/*
 * Bit values for walking two fsms together. Used to constrain what edges
 * will be followed and what combined states can be considered end states.
 *
 * There are four bits because there are four possible states (NEITHER,
 * ONLYA, ONLYB, BOTH). We use these bits as a fast table lookup.
 *
 * When walking two graphs, the caller orders them (A, B). This is
 * important for some non-commutative binary operations, like subtraction,
 * where A-B != B-A.
 *
 * The names below assume that the order is (A, B), but make no assumptions
 * about what that means.
 *
 * These values can be bitwise OR'd together. So, to find the states
 * for subtraction A-B, the mask would be:
 *
 *      FSM_WALK2_ONLYA | FSM_WALK2_BOTH
 */
enum {
	FSM_WALK2_NEITHER = 1 << 0x0,   /* To complete the four possible states */
	FSM_WALK2_ONLYB   = 1 << 0x1,   /* Follow edges that B has and A does not */
	FSM_WALK2_ONLYA   = 1 << 0x2,   /* Follow edges that A has and B does not */
	FSM_WALK2_BOTH    = 1 << 0x3    /* Follow edges that both A and B have */
};

struct fsm *
fsm_walk2(const struct fsm *a, const struct fsm *b,
	unsigned edgemask, unsigned endmask);

#endif

