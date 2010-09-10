/* $Id$ */

#ifndef INTERNAL_H
#define INTERNAL_H

#include <limits.h>

#include <fsm/fsm.h>


/* TODO: +2 for SOL, EOL */
/* TODO: +lots for FSM_EDGE_* */
#define FSM_EDGE_MAX UCHAR_MAX

struct fsm_state {
	unsigned int id;
	int end;

	struct opaque_set *ol;

	struct fsm_state *edges[FSM_EDGE_MAX + 1];
	struct state_set *el;
};


struct opaque_set {
	void *opaque;
	struct opaque_set *next;
};

struct state_set {
	struct fsm_state *state;
	struct state_set *next;
};

struct fsm {
	struct state_set *sl;
	struct fsm_state *start;
	struct fsm_options options;
};

#endif

