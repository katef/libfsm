/* $Id$ */

#ifndef FSM_H
#define FSM_H

struct fsm_state {
	unsigned int id;

	int end;	/* bool */
	struct fsm_edge *edges;
};

struct fsm_edge {
	struct label_list *label;
	struct fsm_state *state;

	struct fsm_edge *next;	/* ll */
};

/* global registry of all states: TODO: can we make this private? */
/* TODO hide behind "fsm_ctx" opaque pointer */
struct state_list {
	struct fsm_state state;
	struct state_list *next;
};

/* global registry of all labels: TODO: ditto */
struct label_list {
	const char *label;
	struct label_list *next;
};

struct fsm_options {
	int anonymous_states;
};

#endif

