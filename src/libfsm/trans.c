/* $Id$ */

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include "internal.h"
#include "trans.h"
#include "xalloc.h"

int
trans_equal(const struct trans_list *a, const struct trans_list *b)
{
	assert(a != NULL);
	assert(b != NULL);

	if (a->type != b->type) {
		return 0;
	}

	switch (a->type) {
	case FSM_EDGE_EPSILON:
	case FSM_EDGE_ANY:
		return 1;

	case FSM_EDGE_LITERAL:
		return a->u.literal == b->u.literal;

	case FSM_EDGE_LABEL:
		assert(a->u.label != NULL);
		assert(b->u.label != NULL);
		return 0 == strcmp(a->u.label, b->u.label);
	}

	assert(!"unreached");
	return 0;
}

struct trans_list *
trans_add(struct fsm *fsm, enum fsm_edge_type type, union trans_value *u)
{
	struct trans_list *t;

	assert(fsm != NULL);

	/* Find an existing transition */
	for (t = fsm->tl; t; t = t->next) {
		struct trans_list tmp;	/* XXX: hacky */

		if (u != NULL) {
			tmp.u    = *u;
		}

		tmp.type = type;
		tmp.next = NULL;

		if (trans_equal(t, &tmp)) {
			return t;
		}
	}

	assert(t == NULL);

	/* Otherwise, create a new one */
	t = malloc(sizeof *t);
	if (t == NULL) {
		return NULL;
	}

	t->type = type;

	switch (type) {
	case FSM_EDGE_EPSILON:
	case FSM_EDGE_ANY:
		break;

	case FSM_EDGE_LITERAL:
	case FSM_EDGE_LABEL:
		assert(u != NULL);
		t->u = *u;
		break;
	}

	t->next = fsm->tl;
	fsm->tl = t;

	return t;
}

int
trans_copyvalue(enum fsm_edge_type type, const union trans_value *from,
	union trans_value *to)
{
	assert(from != NULL);
	assert(to != NULL);

	switch (type) {
	case FSM_EDGE_EPSILON:
	case FSM_EDGE_ANY:
		return 1;

	case FSM_EDGE_LITERAL:
		assert(from != NULL);
		assert(to != NULL);
		to->literal = from->literal;
		return 1;

	case FSM_EDGE_LABEL:
		assert(from != NULL);
		assert(to != NULL);
		to->label = xstrdup(from->label);
		if (to->label == NULL) {
			return 0;
		}
		return 1;
	}

	assert(!"unreached");
	return 0;
}

