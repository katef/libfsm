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

static int
copy_capture_actions(struct fsm *dst, const struct fsm *src);

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
		new->states[i].opaque = fsm->states[i].opaque;

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
		if (!copy_capture_actions(new, fsm)) {
			fsm_free(new);
			return NULL;
		}
	}

	return new;
}

struct copy_capture_actions_env {
	struct fsm *dst;
	int ok;
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
		env->ok = 0;
	}

	return env->ok;
}

static int
copy_capture_actions(struct fsm *dst, const struct fsm *src)
{
	struct copy_capture_actions_env env = { NULL, 1 };
	env.dst = dst;

	fsm_capture_action_iter(src,
	    copy_capture_actions_cb, &env);
	return env.ok;
}

