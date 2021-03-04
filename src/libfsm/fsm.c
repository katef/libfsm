/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include <fsm/alloc.h>
#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/options.h>

#include <adt/alloc.h>
#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "internal.h"
#include "capture.h"
#include "endids.h"

void
free_contents(struct fsm *fsm)
{
	size_t i;

	assert(fsm != NULL);

	for (i = 0; i < fsm->statecount; i++) {
		state_set_free(fsm->states[i].epsilons);
		edge_set_free(fsm->opt->alloc, fsm->states[i].edges);
	}

	fsm_capture_free(fsm);
	fsm_endid_free(fsm);

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
	new->capture_info = NULL;
	new->endid_info = NULL;

	new->states = f_malloc(f.opt->alloc, new->statealloc * sizeof *new->states);
	if (new->states == NULL) {
		f_free(f.opt->alloc, new);
		return NULL;
	}

	fsm_clearstart(new);

	new->opt = opt;

	if (!fsm_capture_init(new)) {
		f_free(f.opt->alloc, new->states);
		f_free(f.opt->alloc, new);
		return NULL;
	}

	if (!fsm_endid_init(new)) {
		f_free(f.opt->alloc, new->states);
		f_free(f.opt->alloc, new);
		fsm_capture_free(new);
		return NULL;
	}

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

	dst->capture_info = src->capture_info;
	dst->endid_info = src->endid_info;

	f_free(src->opt->alloc, src);
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

	for (i = 0; i < fsm->statecount; i++) {
		n += edge_set_count(fsm->states[i].edges);
	}

	return n;
}

