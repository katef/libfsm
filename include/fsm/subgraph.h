/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_SUBGRAPH_H
#define FSM_SUBGRAPH_H

/*
 * Spans nodes added between fsm_subgraph_start() and
 * fsm_subgraph_end()
 *
 * Can be used by fsm_subgraph_duplicate() to make copies of the subgraph.
 *
 * The subgraph capture is only valid as long as no nodes are deleted after
 * fsm_subgraph_start() is called.
 */
struct fsm_subgraph {
	fsm_state_t start;
	fsm_state_t end;
};

void
fsm_subgraph_start(struct fsm *fsm, struct fsm_subgraph *subgraph);

void
fsm_subgraph_stop(struct fsm *fsm, struct fsm_subgraph *subgraph);

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
fsm_subgraph_duplicate(struct fsm *fsm,
	const struct fsm_subgraph *subgraph,
	fsm_state_t *x,
	fsm_state_t *q);

#endif

