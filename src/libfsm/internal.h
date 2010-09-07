/* $Id$ */

#ifndef INTERNAL_H
#define INTERNAL_H

#include <limits.h>


/* TODO: +2 for SOL, EOL */
/* TODO: +lots for FSM_EDGE_* */
#define FSM_EDGE_MAX UCHAR_MAX

struct fsm_state {
	unsigned int id;
	void *opaque;

	int end;

	struct fsm_state *edges[FSM_EDGE_MAX];
	struct epsilon_list *el;
};


/* TODO: observation: same struct as state_list... */
struct epsilon_list {
	struct fsm_state *state;
	struct epsilon_list *next;
};

/* global registry of all states */
struct state_list {
	struct fsm_state state;
	struct state_list *next;
};

struct fsm {
	struct state_list *sl;
	struct fsm_state *start;
	struct fsm_options options;
};

#endif

