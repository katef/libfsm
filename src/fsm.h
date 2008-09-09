/* $Id$ */

#ifndef FSM_H
#define FSM_H

struct fsm_state {
	unsigned int id;

	int end;	/* bool */
	struct fsm_edge *edges;
};

struct fsm_edge {
	const char *label;
	struct fsm_state *state;

	struct fsm_edge *next;	/* ll */
};

/* global registry of all states: TODO: can we make this private? */
/* TODO hide behind "fsm_ctx" opaque pointer */
struct state_list {
	struct fsm_state state;
	struct state_list *next;
};

struct fsm_options {
	int anonymous_states;
};

#endif

