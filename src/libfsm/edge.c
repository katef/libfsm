/* $Id$ */

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include "internal.h"
#include "trans.h"
#include "xalloc.h"

enum fsm_edge_type
fsm_getedgetype(const struct fsm_edge *edge)
{
	assert(edge != NULL);
	assert(edge->trans != NULL);

	return edge->trans->type;
}

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
			e->trans = trans_add(fsm, FSM_EDGE_LITERAL);
			if (e->trans == NULL) {
				return NULL;
			}

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

	/* Assign the literal transition for this edge */
	{
		e->trans = trans_add(fsm, FSM_EDGE_LITERAL);
		if (e->trans == NULL) {
			return NULL;
		}

		e->state = to;
	}

	return e;
}

struct fsm_edge *
fsm_addedge_copy(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to,
	struct fsm_edge *edge)
{
	int i;

	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);
	assert(edge != NULL);
	assert(edge->trans != NULL);

	i = edge - from->edges;

	assert(i >= 0);
	assert(i < FSM_EDGE_MAX);
	assert(to->edges[i].state == NULL);
	assert(to->edges[i].trans == NULL);

	/* Assign the copied transition for this edge */
	{
		to->edges[i].state = edge->state;

		to->edges[i].trans = trans_add(fsm, FSM_EDGE_LITERAL);
		if (to->edges[i].trans == NULL) {
			return NULL;
		}
	}

	return &to->edges[i];
}

