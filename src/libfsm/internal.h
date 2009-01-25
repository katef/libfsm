/* $Id$ */

#ifndef INTERNAL_H
#define INTERNAL_H

struct fsm_state {
	unsigned int id;

	unsigned int end;	/* <= INT_MAX */

	struct fsm_edge *edges;
};

struct fsm_edge {
	struct fsm_state *state;
	struct trans_list *trans;

	struct fsm_edge *next;
};


/* global registry of all states */
struct state_list {
	struct fsm_state state;
	struct state_list *next;
};

enum fsm_edge_type {
	FSM_EDGE_EPSILON,
	FSM_EDGE_ANY,
	FSM_EDGE_LITERAL,
	FSM_EDGE_LABEL
};

union trans_value {
	char literal;
	char *label;
};

/* global registry of all transitions */
struct trans_list {
	enum fsm_edge_type type;
	union trans_value u;

	struct trans_list *next;
};

struct fsm {
	struct state_list *sl;
	struct trans_list *tl;
	struct fsm_state *start;
	struct fsm_options options;
};

#endif

