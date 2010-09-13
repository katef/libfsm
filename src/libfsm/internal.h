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

	struct state_set *edges[FSM_EDGE_MAX + 1];
	struct state_set *el;

	struct fsm_state *next;
};


struct opaque_set {
	void *opaque;
	struct opaque_set *next;
};

struct fsm {
	struct fsm_state *sl;
	struct fsm_state *start;
	struct fsm_options options;
};



/*
 * Add a state of the given id. An existing state is returned if the id is
 * already present.
 *
 * Returns NULL on error; see errno.
 */
struct fsm_state *
fsm_addstateid(struct fsm *fsm, unsigned int id);

/*
 * Find a state by its id. Returns NULL if no start of the given id exists.
 */
struct fsm_state *
fsm_getstatebyid(const struct fsm *fsm, unsigned int id);

/*
 * Find the maximum id.
 *
 * This is intended to be used in conjunction with fsm_getstatebyid() to
 * iterate through all states after renumbering by conversion to a DFA,
 * minimization or similar.
 *
 * Returns 0 if no states are present. TODO: due to be impossible
 */
unsigned int
fsm_getmaxid(const struct fsm *fsm);


#endif

