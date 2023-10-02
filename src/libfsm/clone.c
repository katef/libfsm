/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "internal.h"
#include "capture.h"
#include "endids.h"

#define LOG_CLONE_ENDIDS 0

static int
copy_end_metadata(struct fsm *dst, const struct fsm *src);

struct fsm *
fsm_clone(const struct fsm *fsm)
{
	struct fsm *new;
	size_t i;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	new = fsm_new(fsm->opt);
	if (new == NULL) {
		return NULL;
	}

	if (!fsm_addstate_bulk(new, fsm->statecount)) {
		fsm_free(new);
		return NULL;
	}

	for (i = 0; i < fsm->statecount; i++) {
		if (fsm_isend(fsm, i)) {
			fsm_setend(new, i, 1);
		}

		if (!state_set_copy(&new->states[i].epsilons, new->opt->alloc, fsm->states[i].epsilons)) {
			fsm_free(new);
			return NULL;
		}

		if (!edge_set_copy(&new->states[i].edges, new->opt->alloc, fsm->states[i].edges)) {
			fsm_free(new);
			return NULL;
		}
	}

	{
		fsm_state_t start;

		if (fsm_getstart(fsm, &start)) {
			fsm_setstart(new, start);
		}
	}

	{
		if (!copy_end_metadata(new, fsm)) {
			fsm_free(new);
			return NULL;
		}
	}

	return new;
}

struct copy_end_ids_env {
	char tag;
	struct fsm *dst;
	int ok;
};

static int
copy_end_ids_cb(const struct fsm *fsm, fsm_state_t state,
	size_t nth, const fsm_end_id_t id, void *opaque)
{
	struct copy_end_ids_env *env = opaque;
	enum fsm_endid_set_res sres;
	assert(env->tag == 'c');
	(void)fsm;
	(void)nth;

#if LOG_CLONE_ENDIDS
	fprintf(stderr, "clone[%d] <- %d\n", state, id);
#endif

	sres = fsm_endid_set(env->dst, state, id);
	if (sres == FSM_ENDID_SET_ERROR_ALLOC_FAIL) {
		env->ok = 0;
		return 0;
	}

	return 1;
}

static int
copy_active_capture_ids_cb(fsm_state_t state, unsigned capture_id, void *opaque)
{
	struct copy_end_ids_env *env = opaque;

	if (!fsm_capture_set_active_for_end(env->dst,
		capture_id,
		state)) {
		env->ok = 0;
		return 0;
	}
	return 1;
}

static int
associate_capture_programs_cb(fsm_state_t state, unsigned prog_id, void *opaque)
{
	struct copy_end_ids_env *env = opaque;

	if (!fsm_capture_associate_program_with_end_state(env->dst,
		prog_id, state)) {
		env->ok = 0;
		return 0;
	}
	return 1;
}

static int
copy_end_metadata(struct fsm *dst, const struct fsm *src)
{
	struct copy_end_ids_env env;
	env.tag = 'c';		/* for clone */
	env.dst = dst;
	env.ok = 1;

	fsm_endid_iter(src, copy_end_ids_cb, &env);

	fsm_capture_iter_active_for_all_end_states(src,
	    copy_active_capture_ids_cb, &env);

	if (!fsm_capture_copy_programs(src, dst)) {
		return 0;
	}

	fsm_capture_iter_program_ids_for_all_end_states(src,
	    associate_capture_programs_cb, &env);

	return env.ok;
}
