/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include "internal.h"
#include "capture.h"
#include "endids.h"
#include "eager_output.h"

#define LOG_CLONE_ENDIDS 0

static int
copy_capture_actions(struct fsm *dst, const struct fsm *src);

static int
copy_end_ids(struct fsm *dst, const struct fsm *src);

static int
copy_eager_output_ids(struct fsm *dst, const struct fsm *src);

struct fsm *
fsm_clone(const struct fsm *fsm)
{
	struct fsm *new;
	size_t i;

	assert(fsm != NULL);

	new = fsm_new_statealloc(fsm->alloc, fsm->statecount);
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

		if (!state_set_copy(&new->states[i].epsilons, new->alloc, fsm->states[i].epsilons)) {
			fsm_free(new);
			return NULL;
		}

		if (!edge_set_copy(&new->states[i].edges, new->alloc, fsm->states[i].edges)) {
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
		if (!copy_capture_actions(new, fsm)) {
			fsm_free(new);
			return NULL;
		}

		if (!copy_end_ids(new, fsm)) {
			fsm_free(new);
			return NULL;
		}

		/* does not copy callback */
		if (!copy_eager_output_ids(new, fsm)) {
			fsm_free(new);
			return NULL;
		}
	}

	return new;
}

struct copy_capture_actions_env {
	struct fsm *dst;
	bool ok;
};

static int
copy_capture_actions_cb(fsm_state_t state,
    enum capture_action_type type, unsigned capture_id, fsm_state_t to,
    void *opaque)
{
	struct copy_capture_actions_env *env = opaque;
	assert(env->dst);

	if (!fsm_capture_add_action(env->dst,
		state, type, capture_id, to)) {
		env->ok = false;
	}

	return env->ok;
}

static int
copy_capture_actions(struct fsm *dst, const struct fsm *src)
{
	struct copy_capture_actions_env env = { NULL, true };
	env.dst = dst;

	fsm_capture_action_iter(src,
	    copy_capture_actions_cb, &env);
	return env.ok;
}

struct copy_end_ids_env {
#ifndef NDEBUG
	char tag;
#endif
	struct fsm *dst;
	const struct fsm *src;
	bool ok;
};

static int
copy_end_ids_cb(fsm_state_t state, const fsm_end_id_t id, void *opaque)
{
	struct copy_end_ids_env *env = opaque;
	assert(env->tag == 'c');

#if LOG_CLONE_ENDIDS
	fprintf(stderr, "clone[%d] <- %d\n", state, id);
#endif

	if (!fsm_endid_set(env->dst, state, id)) {
		env->ok = false;
		return 0;
	}

	return 1;
}

static int
copy_end_ids(struct fsm *dst, const struct fsm *src)
{
	struct copy_end_ids_env env;
#ifndef NDEBUG
	env.tag = 'c';		/* for clone */
#endif
	env.dst = dst;
	env.src = src;
	env.ok = true;

	fsm_endid_iter(src, copy_end_ids_cb, &env);

	return env.ok;
}

struct copy_eager_output_ids_env {
	bool ok;
	struct fsm *dst;
};

static int
copy_eager_output_ids_cb(fsm_state_t state, fsm_output_id_t id, void *opaque)
{
	struct copy_eager_output_ids_env *env = opaque;
	if (!fsm_seteageroutput(env->dst, state, id)) {
		env->ok = false;
		return 0;
	}

	return 1;
}

static int
copy_eager_output_ids(struct fsm *dst, const struct fsm *src)
{
	struct copy_eager_output_ids_env env;
	env.dst = dst;
	env.ok = true;

	fsm_eager_output_iter_all(src, copy_eager_output_ids_cb, &env);
	return env.ok;
}
