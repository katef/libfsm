/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include <fsm/fsm.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>
#include <adt/xalloc.h>

#include "internal.h"

int
fsm_addedge_epsilon(struct fsm *fsm,
	fsm_state_t from, fsm_state_t to)
{
	assert(fsm != NULL);
	assert(from < fsm->statecount);
	assert(to < fsm->statecount);

	if (!state_set_add(&fsm->states[from].epsilons, fsm->opt->alloc, to)) {
		return 0;
	}

	return 1;
}

int
fsm_addedge_any(struct fsm *fsm,
	fsm_state_t from, fsm_state_t to)
{
	int i;

	assert(fsm != NULL);
	assert(from < fsm->statecount);
	assert(to < fsm->statecount);

	for (i = 0; i <= UCHAR_MAX; i++) {
		if (!fsm_addedge_literal(fsm, from, to, (unsigned char) i)) {
			return 0;
		}
	}

	return 1;
}

int
fsm_addedge_literal(struct fsm *fsm,
	fsm_state_t from, fsm_state_t to,
	char c)
{
	struct fsm_edge *e;

	assert(fsm != NULL);
	assert(from < fsm->statecount);
	assert(to < fsm->statecount);

	if (fsm->states[from].edges == NULL) {
		fsm->states[from].edges = edge_set_create(fsm->opt->alloc);
		if (fsm->states[from].edges == NULL) {
			return 0;
		}
	}

	e = edge_set_add(fsm->states[from].edges, c, to);
	if (e == NULL) {
		return 0;
	}

	return 1;
}

int
fsm_addedge_bulk(struct fsm *fsm,
	fsm_state_t from, fsm_state_t *a, size_t n,
	char c)
{
	size_t i;

	assert(fsm != NULL);
	assert(a != NULL);

	if (n == 0) {
		return 1;
	}

	if (fsm->states[from].edges == NULL) {
		fsm->states[from].edges = edge_set_create(fsm->opt->alloc);
		if (fsm->states[from].edges == NULL) {
			return 0;
		}
	}

	/* TODO: would hope to have a bulk-add interface for edges */
	for (i = 0; i < n; i++) {
		if (!edge_set_add(fsm->states[from].edges, c, a[i])) {
			return 0;
		}
	}

	return 1;
}

