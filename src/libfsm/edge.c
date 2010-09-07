/* $Id$ */

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include "internal.h"
#include "xalloc.h"

int
fsm_addedge_epsilon(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to)
{
	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);

	/* Assign the epsilon transition for this state */
	{
		struct epsilon_list *e;

		e = malloc(sizeof *e);
		if (e == NULL) {
			return 0;
		}

		e->state = to;

		e->next  = from->el;
		from->el = e;
	}

	return 1;
}

int
fsm_addedge_any(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to)
{
	int i;

	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);

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

	/* TODO: provide for duplicate labels; create an epsilon transition on the fly */

	from->edges[(unsigned char) c] = to;

	return 1;
}

