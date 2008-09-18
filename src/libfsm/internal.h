/* $Id$ */

#ifndef INTERNAL_H
#define INTERNAL_H

struct fsm_state {
	unsigned int id;

	unsigned int end:1;    /* boolean */
	struct fsm_edge *edges;
};

struct fsm_edge {
	struct label_list *label;
	struct fsm_state *state;

	struct fsm_edge *next;
};


/* global registry of all states */
struct state_list {
	struct fsm_state state;
	struct state_list *next;
};

/* global registry of all labels */
struct label_list {
	const char *label;
	struct label_list *next;
};

struct fsm {
	struct state_list *sl;
	struct label_list *ll;
	struct fsm_state *start;
	struct fsm_options options;
};

#endif

