/* $Id$ */

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include "internal.h"
#include "set.h"
#include "xalloc.h"

int
fsm_addedge_epsilon(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to)
{
	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);

	(void) fsm;

	return !!set_addstate(&from->el, to);
}

int
fsm_addedge_any(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to)
{
	int i;

	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);

	(void) fsm;

	for (i = 0; i <= UCHAR_MAX; i++) {
		from->edges[i] = to;
	}

	return 1;
}

int
fsm_addedge_literal(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to,
	char c)
{
	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);

	/*
	 * NFA may have duplicate edges. Since edges are stored in an array indexed
	 * by c, duplicate transitions cannot be expressed directly. Instead, we
	 * create a dummy epsilon transition which serves to hold a newly-created
	 * state, and transition from that state instead.
	 */
	if (from->edges[(unsigned char) c] != NULL) {
		struct fsm_state *new;

		/*
		 * XXX: this causes strange effects when the user doesn't expect state
		 * numbering to change. have inventid() pick descending values from
		 * UINT_MAX instead
		 */
		new = fsm_addstate(fsm, 0);
		if (new == NULL) {
			return 0;
		}

		if (!fsm_addedge_epsilon(fsm, from, new)) {
			return 0;
		}

		from = new;
	}

	from->edges[(unsigned char) c] = to;

	return 1;
}

