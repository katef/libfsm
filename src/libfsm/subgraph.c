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
			for (state_set_reset(fsm->states[m->old].epsilons, &jt); state_set_next(&jt, &s); ) {
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
		for (e = edge_set_first(fsm->states[m->old].edges, &it); e != NULL; e = edge_set_next(&it)) {
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

void
fsm_subgraph_capture_start(struct fsm *fsm, struct fsm_subgraph_capture *capture)
{
	assert(fsm != NULL);
	assert(capture != NULL);

	capture->start = fsm_countstates(fsm);
}

void
fsm_subgraph_capture_stop(struct fsm *fsm, struct fsm_subgraph_capture *capture)
{
	assert(fsm != NULL);
	assert(capture != NULL);

	capture->end = fsm_countstates(fsm);
}

/* As with fsm_state_duplicatesubgraphx(), if x is non-NULL, *x must be a state
 * in the captured subgraph.  *x is overwritten with its equivalent state in
 * the duplicated subgraph.
 *
 * *q is the start of the subgraph.
 */
int
fsm_subgraph_capture_duplicate(struct fsm *fsm,
	const struct fsm_subgraph_capture *capture,
	fsm_state_t *x,
	fsm_state_t *q)
{
	fsm_state_t old_start, old_end;
	fsm_state_t new_start, new_end;
	fsm_state_t ind;

	assert(fsm     != NULL);
	assert(capture != NULL);
	assert(q       != NULL);
	assert(x == NULL || (*x >= capture->start && *x < capture->end));

	old_start = capture->start;
	old_end   = capture->end;

	if (old_start >= old_end) {
		return 0;
	}

	/* allocate new states */
	new_start = new_end = 0;
	for (ind = old_start; ind < old_end; ind++) {
		fsm_state_t st;
		if (!fsm_addstate(fsm, &st)) {
			return 0;
		}

		fsm_setend(fsm, st, fsm_isend(fsm, ind));

		if (ind == old_start) {
			new_start = st;
		}

		new_end = st+1;
	}

	if (new_start == new_end) {
		return 0;
	}

	/* allocate edges */
	for (ind = new_start; ind < new_end; ind++) {
		const fsm_state_t old_src = old_start + (ind - new_start);
		struct edge_iter it;
		struct fsm_edge *e;

		{
			struct state_iter jt;
			fsm_state_t old_dst;

			for (state_set_reset(fsm->states[old_src].epsilons, &jt); state_set_next(&jt, &old_dst); ) {
				fsm_state_t new_dst;

				if (old_dst < old_start || old_dst >= old_end) {
					continue;
				}

				new_dst = new_start + (old_dst - old_start);

				if (!fsm_addedge_epsilon(fsm, ind, new_dst)) {
					return 0;
				}
			}
		}

		for (e = edge_set_first(fsm->states[old_src].edges, &it); e != NULL; e = edge_set_next(&it)) {
			struct state_iter jt;
			fsm_state_t old_dst;

			for (state_set_reset(e->sl, &jt); state_set_next(&jt, &old_dst); ) {
				fsm_state_t new_dst;

				if (old_dst < old_start || old_dst >= old_end) {
					continue;
				}

				new_dst = new_start + (old_dst - old_start);

				if (!fsm_addedge_literal(fsm, ind, new_dst, e->symbol)) {
					return 0;
				}
			}
		}
	}

	if (x != NULL && *x >= old_start && *x < old_end) {
		fsm_state_t old_x = *x;
		*x = new_start + (old_x - old_start);
	}

	*q = new_start;

	return 1;
}

