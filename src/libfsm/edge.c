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

struct fsm_edge *
fsm_addedge_any(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to)
{
	struct fsm_edge *e;
	int i;

	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);

	for (i = 0; i <= UCHAR_MAX; i++) {
		e = &from->edges[i];

		/* Assign the any transition for this edge */
		{
			e->state = to;
		}
	}

	return e;
}

struct fsm_edge *
fsm_addedge_literal(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to,
	char c)
{
	struct fsm_edge *e;

	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);

	e = &from->edges[(unsigned char) c];

	/* TODO: provide for duplicate labels; create an epsilon transition on the fly */

	/* Assign the literal transition for this edge */
	{
		e->state = to;
	}

	return e;
}

