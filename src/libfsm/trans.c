/* $Id$ */

#include <assert.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include "internal.h"
#include "trans.h"
#include "xalloc.h"

/* TODO: pass in index? find index somehow? find all instances and compare outside there */
int
trans_equal(const struct trans_list *a, const struct trans_list *b)
{
	assert(a != NULL);
	assert(b != NULL);

	if (a->type != b->type) {
		return 0;
	}

	switch (a->type) {
	case FSM_EDGE_LITERAL:
		return 1;
	}

	assert(!"unreached");
	return 0;
}

/* TODO: try to get rid of trans types. type can depend on edge[] location.
 * if not a normal char, it must be epsilon: easy to identify. */
struct trans_list *
trans_add(struct fsm *fsm, enum fsm_edge_type type)
{
	struct trans_list *t;

	assert(fsm != NULL);

	/* Find an existing transition */
	for (t = fsm->tl; t; t = t->next) {
		struct trans_list tmp;	/* XXX: hacky */

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
	t->next = fsm->tl;
	fsm->tl = t;

	return t;
}

