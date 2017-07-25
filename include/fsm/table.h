#ifndef FSM_TABLE_H
#define FSM_TABLE_H

#include <stdlib.h>

struct fsm;

struct fsm_table {
	size_t nstates;		/* number of states */
	size_t start;		/* index of start state, or (size_t)-1 */

	size_t nend;		/* number of end edges */
	struct {
		size_t state;
		void *opaque;	/* optional opaque value associated with end states */
	} *endstates;		/* array of opaque values */

	size_t nedges;		/* number of transitions */
	struct {
		size_t src;
		unsigned int lbl;
		size_t dst;
	} *edges;		/* array of edges */
};

/* Converts the FSM into adjacency lists.  Includes information about
 * the start state, the end states, and opaque values associated with
 * states.
 */
struct fsm_table *
fsm_table(const struct fsm *fsm);

void
fsm_table_free(struct fsm_table *t);

#endif

