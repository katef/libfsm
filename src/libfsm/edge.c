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

struct fsm_edge *
fsm_addedge_epsilon(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to)
{
	struct fsm_edge *e;

	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);

	e = &from->edges[FSM_EDGE_EPSILON];

	/* Assign the epsilon transition for this edge */
	{
		e->trans = trans_add(fsm, FSM_EDGE_EPSILON, NULL);
		if (e->trans == NULL) {
			return NULL;
		}

		e->state = to;
	}

	return e;
}

struct fsm_edge *
fsm_addedge_any(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to)
{
	struct fsm_edge *e;

	assert(fsm != NULL);
	assert(from != NULL);
	assert(to != NULL);

	e = &from->edges[FSM_EDGE_ANY];

	/* Assign the any transition for this edge */
	{
		e->trans = trans_add(fsm, FSM_EDGE_ANY, NULL);
		if (e->trans == NULL) {
			return NULL;
		}

		e->state = to;
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
	assert((unsigned char) c >= 0);
	assert((unsigned char) c <= FSM_EDGE_MAX);

	e = &from->edges[(unsigned char) c];

	/* Assign the literal transition for this edge */
	{
		union trans_value u;

		u.literal = c;
		e->trans = trans_add(fsm, FSM_EDGE_LITERAL, &u);
		if (e->trans == NULL) {
			return NULL;
		}

		e->state = to;
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

	e = &from->edges[(unsigned char) label[0]];	/* XXX: hacky. due to go when labels go */

	/* Assign the labelled transition for this edge */
	{
		union trans_value u;

		u.label = xstrdup(label);
		if (u.label == NULL) {
			return NULL;
		}

		e->trans = trans_add(fsm, FSM_EDGE_LABEL, &u);
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
		union trans_value u;

		if (!trans_copyvalue(edge->trans->type, &edge->trans->u, &u)) {
			return NULL;
		}

		to->edges[i].state = edge->state;

		to->edges[i].trans = trans_add(fsm, edge->trans->type, &u);
		if (to->edges[i].trans == NULL) {
			return NULL;
		}
	}

	return &to->edges[i];
}

