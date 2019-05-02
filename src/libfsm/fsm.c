/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include <adt/alloc.h>
#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include <fsm/alloc.h>
#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/options.h>

#include "internal.h"

#define ctassert(pred) \
	switch (0) { case 0: case (pred):; }

void
free_contents(struct fsm *fsm)
{
	size_t i;

	assert(fsm != NULL);

	for (i = 0; i < fsm->statecount; i++) {
		struct edge_iter it;
		struct fsm_edge *e;

		for (e = edge_set_first(fsm->states[i]->edges, &it); e != NULL; e = edge_set_next(&it)) {
			state_set_free(e->sl);
			f_free(fsm->opt->alloc, e);
		}

		state_set_free(fsm->states[i]->epsilons);
		edge_set_free(fsm->states[i]->edges);
		f_free(fsm->opt->alloc, fsm->states[i]);
	}

	f_free(fsm->opt->alloc, fsm->states);
}

struct fsm *
fsm_new(const struct fsm_options *opt)
{
	static const struct fsm_options defaults;
	struct fsm *new, f;

	if (opt == NULL) {
		opt = &defaults;
	}

	f.opt = opt;

	new = f_malloc(f.opt->alloc, sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->statealloc = 128; /* guess */
	new->statecount = 0;
	new->endcount   = 0;

	new->states = f_malloc(f.opt->alloc, new->statealloc * sizeof *new->states);
	if (new->states == NULL) {
		f_free(f.opt->alloc, new);
		return NULL;
	}

	new->start  = NULL;

	new->opt = opt;

	return new;
}

void
fsm_free(struct fsm *fsm)
{
	assert(fsm != NULL);

	free_contents(fsm);

	f_free(fsm->opt->alloc, fsm);
}

const struct fsm_options *
fsm_getoptions(const struct fsm *fsm)
{
	return fsm->opt;
}

void
fsm_setoptions(struct fsm *fsm, const struct fsm_options *opts)
{
	fsm->opt = opts;
}

void
fsm_move(struct fsm *dst, struct fsm *src)
{
	assert(src != NULL);
	assert(dst != NULL);

	if (dst->opt != src->opt) {
		errno = EINVAL;
		return; /* XXX */
	}

	free_contents(dst);

	dst->states     = src->states;
	dst->start      = src->start;
	dst->statecount = src->statecount;
	dst->statealloc = src->statealloc;
	dst->endcount   = src->endcount;

	f_free(src->opt->alloc, src);
}

void
fsm_carryopaque_array(struct fsm *src_fsm, const struct fsm_state **src_set, size_t n,
	struct fsm *dst_fsm, struct fsm_state *dst_state)
{
	ctassert(sizeof (void *) == sizeof (struct fsm_state *));

	assert(src_fsm != NULL);
	assert(src_set != NULL);
	assert(n > 0);
	assert(dst_fsm != NULL);
	assert(fsm_isend(dst_fsm, dst_state));
	assert(src_fsm->opt == dst_fsm->opt);

	/* 
	 * Some states in src_set may be not end states (for example
	 * from an epsilon closure over a mix of end and non-end states).
	 * However at least one element is known to be an end state,
	 * so we assert on that here.
	 *
	 * I would filter out the non-end states if there were a convenient
	 * way to do that without allocating for it. As it is, the caller
	 * must unfortunately be exposed to a mix.
	 */
#ifndef NDEBUG
	{
		int has_end;
		size_t i;

		has_end = 0;

		for (i = 0; i < n; i++) {
			if (fsm_isend(src_fsm, src_set[i])) {
				has_end = 1;
				break;
			}
		}

		assert(has_end);
	}
#endif

	if (src_fsm->opt == NULL || src_fsm->opt->carryopaque == NULL) {
		return;
	}

	src_fsm->opt->carryopaque(src_fsm, src_set, n,
		dst_fsm, dst_state);
}

void
fsm_carryopaque(struct fsm *src_fsm, const struct state_set *src_set,
	struct fsm *dst_fsm, struct fsm_state *dst_state)
{
	assert(src_fsm != NULL);
	assert(dst_fsm != NULL);
	assert(fsm_isend(dst_fsm, dst_state));
	assert(src_fsm->opt == dst_fsm->opt);

	ctassert(sizeof (void *) == sizeof (struct fsm_state *));

	/* TODO: right? */
	if (state_set_empty(src_set)) {
		return;
	}

	/*
	 * Our sets are a void ** treated as an array of elements of type void *.
	 * Here we're presenting these as if they're an array of elements
	 * of type struct fsm_state *.
	 *
	 * This is not portable because the representations of those types
	 * need not be compatible in general, hence the compile-time assert
	 * and the cast here.
	 */

	fsm_carryopaque_array(src_fsm, (void *) state_set_array(src_set), state_set_count(src_set),
		dst_fsm, dst_state);
}

unsigned int
fsm_countstates(const struct fsm *fsm)
{
	assert(fsm != NULL);

	return fsm->statecount;
}

unsigned int
fsm_countedges(const struct fsm *fsm)
{
	unsigned int n = 0;
	size_t i;

	/*
	 * XXX - this counts all src,lbl,dst tuples individually and
	 * should be replaced with something better when possible
	 */
	for (i = 0; i < fsm->statecount; i++) {
		struct edge_iter ei;
		const struct fsm_edge *e;

		for (e = edge_set_first(fsm->states[i]->edges, &ei); e != NULL; e=edge_set_next(&ei)) {
			assert(n + state_set_count(e->sl) > n); /* handle overflow with more grace? */
			n += state_set_count(e->sl);
		}
	}

	return n;
}

void
fsm_clear_tmp(struct fsm *fsm)
{
	size_t i;

	assert(fsm != NULL);

	for (i = 0; i < fsm->statecount; i++) {
		fsm_state_clear_tmp(fsm->states[i]);
	}
}

