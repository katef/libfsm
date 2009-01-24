/* $Id$ */

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include "internal.h"
#include "trans.h"
#include "xalloc.h"

enum fsm_edge_type
fsm_getedgetype(struct fsm_edge *edge)
{
	assert(edge != NULL);
	assert(edge->trans != NULL);

	return edge->trans->type;
}

static struct fsm_edge*
addedge(struct fsm_state *from, struct fsm_state *to)
{
	struct fsm_edge *e;

	assert(from != NULL);
	assert(to != NULL);

	e = malloc(sizeof *e);
	if (e == NULL) {
		return NULL;
	}

	e->state = to;
	e->trans = NULL;

	e->next  = from->edges;
	from->edges = e;

	return e;
}

static void
freeedge(struct fsm_state *from, struct fsm_edge *e)
{
	assert(from != NULL);
	assert(e != NULL);
	assert(e->trans == NULL);

	/* assumed to be called immediately after addedge() */
	from->edges = e->next;
	free(e);
}

struct fsm_edge *
fsm_addedge_epsilon(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to)
{
	struct fsm_edge *e;

	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);

	e = addedge(from, to);
	if (e == NULL) {
		return NULL;
	}

	/* Assign the epsilon transition for this edge */
	{
		e->trans = trans_add(fsm, FSM_EDGE_EPSILON, NULL);
		if (e->trans == NULL) {
			freeedge(from, e);
			return NULL;
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

	e = addedge(from, to);
	if (e == NULL) {
		return NULL;
	}

	/* Assign the literal transition for this edge */
	{
		union trans_value u;

		u.literal = c;
		e->trans = trans_add(fsm, FSM_EDGE_LITERAL, &u);
		if (e->trans == NULL) {
			freeedge(from, e);
			return NULL;
		}
	}

	return e;
}

struct fsm_edge *
fsm_addedge_label(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to,
	const char *label)
{
	struct fsm_edge *e;

	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);
	assert(label != NULL);
	assert(*label != '\0');

	/* TODO: consider permitting from and to states to be NULL for API convenience (create them on the fly) */

	/* TODO: why do we not allow strlen(label) == 0? */
	/* TODO: is this neccessary given the above assertion indicates it's UB? */
	if (label != NULL && strlen(label) == 0) {
		/* TODO: set errno for things like this */
		return NULL;
	}

	e = addedge(from, to);
	if (e == NULL) {
		return NULL;
	}

	/* Assign the labelled transition for this edge */
	{
		union trans_value u;

		u.label = xstrdup(label);
		if (u.label == NULL) {
			freeedge(from, e);
			return NULL;
		}

		e->trans = trans_add(fsm, FSM_EDGE_LABEL, &u);
		if (e->trans == NULL) {
			freeedge(from, e);
			return NULL;
		}
	}

	return e;
}

struct fsm_edge *
fsm_addedge_copy(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to,
	struct fsm_edge *edge)
{
	struct fsm_edge *e;

	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);
	assert(edge != NULL);
	assert(edge->trans != NULL);

	e = addedge(from, to);
	if (e == NULL) {
		return NULL;
	}

	/* Assign the copied transition for this edge */
	{
		union trans_value u;

		if (!trans_copyvalue(edge->trans->type, &edge->trans->u, &u)) {
			freeedge(from, e);
			return NULL;
		}

		e->trans = trans_add(fsm, edge->trans->type, &u);
		if (e->trans == NULL) {
			freeedge(from, e);
			return NULL;
		}
	}

	return e;
}

