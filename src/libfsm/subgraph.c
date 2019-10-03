/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include <adt/alloc.h>
#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>
#include <adt/xalloc.h>

#include "internal.h"

struct mapping {
	fsm_state_t old;
	fsm_state_t new;

	unsigned int done:1;

	struct mapping *next;
};

static struct mapping *
mapping_ensure(struct fsm *fsm, struct mapping **head, fsm_state_t old)
{
	struct mapping *m;

	assert(fsm != NULL);
	assert(head != NULL);
	assert(old < fsm->statecount);

	/* Find an existing mapping */
	for (m = *head; m != NULL; m = m->next) {
		if (m->old == old) {
			return m;
		}
	}

	/* Otherwise, make a new one */
	{
		m = f_malloc(fsm->opt->alloc, sizeof *m);
		if (m == NULL) {
			return 0;
		}

		if (!fsm_addstate(fsm, &m->new)) {
			f_free(fsm->opt->alloc, m);
			return 0;
		}

		fsm_setend(fsm, m->new, fsm_isend(fsm, old));

		m->old  = old;
		m->done = 0;

		m->next = *head;
		*head = m;
	}

	return m;
}

static void
mapping_free(const struct fsm *fsm, struct mapping *mapping)
{
	struct mapping *next;
	struct mapping *m;

	for (m = mapping; m != NULL; m = next) {
		next = m->next;

		f_free(fsm->opt->alloc, m);
	}
}

static struct mapping *
getnextnotdone(struct mapping *mapping)
{
	struct mapping *m;

	assert(mapping != NULL);

	for (m = mapping; m != NULL; m = m->next) {
		if (!m->done) {
			return m;
		}
	}

	return NULL;
}

int
fsm_state_duplicatesubgraph(struct fsm *fsm, fsm_state_t state,
	fsm_state_t *q)
{
	assert(fsm != NULL);
	assert(state < fsm->statecount);
	assert(q != NULL);

	return fsm_state_duplicatesubgraphx(fsm, state, NULL, q);
}

int
fsm_state_duplicatesubgraphx(struct fsm *fsm, fsm_state_t state,
	fsm_state_t *x,
	fsm_state_t *q)
{
	struct mapping *mappings;
	struct mapping *m;
	struct mapping *start;

	assert(fsm != NULL);
	assert(state < fsm->statecount);
	assert(q != NULL);

	mappings = NULL;

	/* Start off the working list by populating it with the given state */
	start = mapping_ensure(fsm, &mappings, state);
	if (start == NULL) {
		return 0;
	}

	/* TODO: does this traversal algorithim have a name? */
	/* TODO: errors leave fsm in a questionable state */

	while (m = getnextnotdone(mappings), m != NULL) {
		struct edge_iter it;
		struct state_iter jt;
		struct fsm_edge *e;
		fsm_state_t s;

		if (x != NULL && m->old == *x) {
			*x = m->new;
		}

		{
			for (state_set_reset(fsm->states[m->old]->epsilons, &jt); state_set_next(&jt, &s); ) {
				struct mapping *to;

				to = mapping_ensure(fsm, &mappings, s);
				if (to == NULL) {
					mapping_free(fsm, mappings);
					return 0;
				}

				if (!fsm_addedge_epsilon(fsm, m->new, to->new)) {
					return 0;
				}
			}
		}
		for (e = edge_set_first(fsm->states[m->old]->edges, &it); e != NULL; e = edge_set_next(&it)) {
			for (state_set_reset(e->sl, &jt); state_set_next(&jt, &s); ) {
				struct mapping *to;

				to = mapping_ensure(fsm, &mappings, s);
				if (to == NULL) {
					mapping_free(fsm, mappings);
					return 0;
				}

				if (!fsm_addedge_literal(fsm, m->new, to->new, e->symbol)) {
					return 0;
				}
			}
		}

		m->done = 1;
	}

	*q = start->new;
	mapping_free(fsm, mappings);

	return 1;
}

