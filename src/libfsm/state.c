/* $Id$ */

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>

#include <adt/set.h>
#include <adt/xalloc.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include "internal.h"

struct mapping {
	struct fsm_state *old;
	struct fsm_state *new;

	unsigned int done:1;

	struct mapping *next;
};

static struct mapping *
mapping_ensure(struct fsm *fsm, struct mapping **head, struct fsm_state *old)
{
	struct mapping *m;

	assert(fsm != NULL);
	assert(head != NULL);
	assert(old != NULL);

	/* Find an existing mapping */
	for (m = *head; m != NULL; m = m->next) {
		if (m->old == old) {
			return m;
		}
	}

	/* Otherwise, make a new one */
	{
		m = malloc(sizeof *m);
		if (m == NULL) {
			return 0;
		}

		m->new = fsm_addstate(fsm);
		if (m->new == NULL) {
			free(m);
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
mapping_free(struct mapping *mapping)
{
	struct mapping *next;
	struct mapping *m;

	for (m = mapping; m != NULL; m = next) {
		next = m->next;

		free(m);
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

struct fsm_state *
fsm_state_duplicatesubgraph(struct fsm *fsm, struct fsm_state *state)
{
	assert(fsm != NULL);
	assert(state != NULL);

	return fsm_state_duplicatesubgraphx(fsm, state, NULL);
}

struct fsm_state *
fsm_state_duplicatesubgraphx(struct fsm *fsm, struct fsm_state *state,
	struct fsm_state **x)
{
	struct mapping *mappings;
	struct mapping *m;
	struct mapping *start;

	assert(fsm != NULL);
	assert(state != NULL);

	mappings = NULL;

	/* Start off the working list by populating it with the given state */
	start = mapping_ensure(fsm, &mappings, state);
	if (start == NULL) {
		return 0;
	}

	/* TODO: does this traversal algorithim have a name? */
	/* TODO: errors leave fsm in a questionable state */

	while ((m = getnextnotdone(mappings)) != NULL) {
		struct state_set *s;
		int i;

		if (x != NULL && m->old == *x) {
			*x = m->new;
		}

		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			for (s = m->old->edges[i].sl; s != NULL; s = s->next) {
				struct mapping *to;

				assert(s->state != NULL);

				to = mapping_ensure(fsm, &mappings, s->state);
				if (to == NULL) {
					mapping_free(mappings);
					return NULL;
				}

				/* TODO: this switch occurs in a few places; centralise it */
				switch (i) {
				case FSM_EDGE_EPSILON:
					if (!fsm_addedge_epsilon(fsm, m->new, to->new)) {
						mapping_free(mappings);
						return NULL;
					}
					break;

				default:
					if (!fsm_addedge_literal(fsm, m->new, to->new, i)) {
						mapping_free(mappings);
						return NULL;
					}
					break;
				}
			}
		}

		m->done = 1;
	}

	mapping_free(mappings);

	return start->new;
}

struct fsm_state *
fsm_mergestates(struct fsm *fsm, struct fsm_state *a, struct fsm_state *b)
{
	int i;

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		/* there should be no common elements, because a and b are from different FSMs */
		/* TODO: make a set_distinct() or set_disjoint() or somesuch */
		assert(a->edges[i].sl == NULL || !subsetof(a->edges[i].sl, b->edges[i].sl));
		assert(b->edges[i].sl == NULL || !subsetof(b->edges[i].sl, a->edges[i].sl));

		set_merge(&a->edges[i].sl, b->edges[i].sl);

		b->edges[i].sl = NULL;
	}

	/* TODO: could just use internal state_remove(), since no edges transition to to b */
	fsm_removestate(fsm, b);

	return a;
}

