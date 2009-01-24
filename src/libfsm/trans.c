/* $Id$ */

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include "internal.h"
#include "trans.h"
#include "xalloc.h"

int
trans_equal(enum fsm_edge_type type, union trans_value *a, union trans_value *b)
{
	switch (type) {
	case FSM_EDGE_EPSILON:
		return 1;

	case FSM_EDGE_LITERAL:
		assert(a != NULL);
		assert(b != NULL);
		return a->literal == b->literal;

	case FSM_EDGE_LABEL:
		assert(a != NULL);
		assert(b != NULL);
		return 0 == strcmp(a->label, b->label);
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
		if (t->type != type) {
			continue;
		}

		if (trans_equal(type, &t->u, u)) {
			return t;
		}
	}

	assert(t == NULL);

	/* Otherwise, create a new one */
	t = malloc(sizeof *t);
	if (t == NULL) {
		return NULL;
	}

	t->next = fsm->tl;
	fsm->tl = t;

	if (type != FSM_EDGE_EPSILON) {
		assert(u != NULL);
		t->type = type;
		t->u = *u;
	}

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

