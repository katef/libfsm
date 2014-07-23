/* $Id$ */

#ifndef FSM_SEARCH_H
#define FSM_SEARCH_H

struct fsm;
struct fsm_state;

/*
 * Find the least-cost ("shortest") path between two states.
 *
 * A discovered path is returned, or NULL on error. If the goal is not
 * reachable, then the path returned will be non-NULL but will not contain
 * the goal state.
 */
struct priq *
fsm_shortest(const struct fsm *fsm,
	const struct fsm_state *start, const struct fsm_state *goal,
	unsigned (*cost)(const struct fsm_state *from, const struct fsm_state *to, int c));

#endif

