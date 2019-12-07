/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_CAPTURE_H
#define FSM_CAPTURE_H

/*
 * Captures nodes added between fsm_capture_start() and
 * fsm_capture_end()
 *
 * Can be used by fsm_capture_duplicate() to make copies of the subgraph.
 *
 * The subgraph capture is only valid as long as no nodes are deleted after
 * fsm_capture_start() is called.
 */
struct fsm_capture {
	fsm_state_t start;
	fsm_state_t end;
};

void
fsm_capture_start(struct fsm *fsm, struct fsm_capture *capture);

void
fsm_capture_stop(struct fsm *fsm, struct fsm_capture *capture);

/*
 * Duplicate a captured sub-graph.
 *
 * If non-NULL, the state x in the origional subgraph is overwritten with its
 * equivalent state in the duplicated subgraph.
 * This provides a mechanism to keep track of a state of interest (for example
 * the endpoint of a segment).
 *
 * The start of the subgraph is output to *q.
 */
int
fsm_capture_duplicate(struct fsm *fsm,
	const struct fsm_capture *capture,
	fsm_state_t *x,
	fsm_state_t *q);

#endif

