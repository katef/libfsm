/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_BOOL_H
#define FSM_BOOL_H

/*
 * Boolean operators.
 *
 * The resulting FSM may be an NFA.
 *
 * Binary operators are commutative. They are also destructive,
 * and free their operands as if by fsm_free().
 */

struct fsm;

int
fsm_complement(struct fsm *fsm);

struct fsm *
fsm_union(struct fsm *a, struct fsm *b);

struct fsm *
fsm_intersect(struct fsm *a, struct fsm *b);

/*
 * Subtract b from a. This is not commutative.
 */
struct fsm *
fsm_subtract(struct fsm *a, struct fsm *b);

#endif

