/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/capture.h>

#include <adt/set.h>
#include <adt/edgeset.h>
#include <adt/stateset.h>

#include "capture.h"
#include "internal.h"
#include "endids.h"

#define LOG_MERGE_ENDIDS 0

struct copy_capture_env {
	char tag;
	struct fsm *dst;
	int ok;
};

static int
copy_capture_actions(struct fsm *dst, struct fsm *src);

static int
copy_end_ids(struct fsm *dst, struct fsm *src, fsm_state_t base_src);

static struct fsm *
merge(struct fsm *dst, struct fsm *src,
	fsm_state_t *base_dst, fsm_state_t *base_src,
	unsigned *capture_base_dst, unsigned *capture_base_src)
{
	assert(dst != NULL);
	assert(src != NULL);
	assert(base_src != NULL);
	assert(base_dst != NULL);

	if (dst->statealloc < src->statecount + dst->statecount) {
		void *tmp;

		size_t newalloc = src->statecount + dst->statecount;

		/* TODO: round up to next power of two here?
		 * or let realloc do that internally */
		tmp = f_realloc(dst->opt->alloc, dst->states, newalloc * sizeof *dst->states);
		if (tmp == NULL) {
			return NULL;
		}

		dst->states = tmp;
		dst->statealloc = newalloc;
	}

	/*
	 * XXX: base_a and base_b would become redundant if we change to the
	 * shared global array idea.
	 */
	{
		fsm_state_t i;

		*base_dst = 0;
		*base_src = dst->statecount;
		*capture_base_dst = 0;
		*capture_base_src = fsm_countcaptures(dst);

		for (i = 0; i < src->statecount; i++) {
			state_set_rebase(&src->states[i].epsilons, *base_src);
			edge_set_rebase(&src->states[i].edges, *base_src);
		}

		/* FIXME: instead of rebasing these here, they could
		 * also be updated in copy_capture_actions below. */
		fsm_capture_rebase_capture_id(src, *capture_base_src);
		fsm_capture_rebase_capture_action_states(src, *base_src);
	}

	memcpy(dst->states + dst->statecount, src->states,
		src->statecount * sizeof *src->states);
	dst->statecount += src->statecount;
	dst->endcount   += src->endcount;

	/* We need to explicitly copy over the capture actions and end
	 * ID info here because they're stored on the FSMs as a whole,
	 * rather than individual states; `memcpy`ing the states alone
	 * won't transfer them.
	 *
	 * They're stored separately because they are likely to only
	 * be on a small portion of the states, and adding two extra
	 * NULL pointers to `struct fsm_state` increases memory usage
	 * significantly. */

	if (!copy_capture_actions(dst, src)) {
		/* non-recoverable -- destructive operation */
		return NULL;
	}

	if (!copy_end_ids(dst, src, *base_src)) {
		/* non-recoverable -- destructive operation */
		return NULL;
	}

	f_free(src->opt->alloc, src->states);
	src->states = NULL;
	src->statealloc = 0;
	src->statecount = 0;

	fsm_clearstart(src);
	fsm_clearstart(dst);

	fsm_free(src);

	return dst;
}

static int
copy_capture_cb(fsm_state_t state,
    enum capture_action_type type, unsigned capture_id, fsm_state_t to,
    void *opaque)
{
	struct copy_capture_env *env = opaque;
	assert(env->tag == 'C');

	if (!fsm_capture_add_action(env->dst, state, type,
		capture_id, to)) {
		env->ok = 0;
		return 0;
	}

	return 1;
}

static int
copy_capture_actions(struct fsm *dst, struct fsm *src)
{
	struct copy_capture_env env;
	env.tag = 'C';
	env.dst = dst;
	env.ok = 1;

	fsm_capture_action_iter(src, copy_capture_cb, &env);

	return env.ok;
}

struct copy_end_ids_env {
	char tag;
	struct fsm *dst;
	struct fsm *src;
	fsm_state_t base_src;
	int ok;
};

static int
copy_end_ids_cb(fsm_state_t state, fsm_end_id_t id, void *opaque)
{
	enum fsm_endid_set_res sres;
	struct copy_end_ids_env *env = opaque;
	assert(env->tag == 'M');

#if LOG_MERGE_ENDIDS > 1
	fprintf(stderr, "merge[%d] <- %d\n",
	    state + env->base_src, id);
#endif

	sres = fsm_endid_set(env->dst, state + env->base_src, id);
	if (sres == FSM_ENDID_SET_ERROR_ALLOC_FAIL) {
		env->ok = 0;
		return 0;
	}

	return 1;
}

static int
copy_end_ids(struct fsm *dst, struct fsm *src, fsm_state_t base_src)
{
	struct copy_end_ids_env env;
	env.tag = 'M';		/* for Merge */
	env.dst = dst;
	env.src = src;
	env.base_src = base_src;
	env.ok = 1;

	fsm_endid_iter(src, copy_end_ids_cb, &env);
	return env.ok;
}

struct fsm *
fsm_mergeab(struct fsm *a, struct fsm *b,
	fsm_state_t *base_b)
{
	fsm_state_t dummy; /* always 0 */
	unsigned capture_base_a, capture_base_b; /* unused */
	struct fsm *q;

	assert(a != NULL);
	assert(b != NULL);
	assert(base_b != NULL);

	if (a->opt != b->opt) {
		errno = EINVAL;
		return NULL;
	}

	/*
	 * We merge b into a.
	 */

	q = merge(a, b, &dummy, base_b,
	    &capture_base_a, &capture_base_b);

	assert(dummy == 0);

	return q;
}

struct fsm *
fsm_merge(struct fsm *a, struct fsm *b,
	struct fsm_combine_info *combine_info)
{
	int a_dst;
	struct fsm *res;

	assert(a != NULL);
	assert(b != NULL);
	assert(combine_info != NULL);

	if (a->opt != b->opt) {
		errno = EINVAL;
		return NULL;
	}

	/*
	 * We merge the smaller FSM into the larger FSM.
	 * The number of allocate states is considered first, because realloc
	 * is probably more expensive than memcpy.
	 */

	if (a->statealloc > b->statealloc) {
		a_dst = 1;
	} else if (a->statealloc < b->statealloc) {
		a_dst = 0;
	} else if (a->statecount > b->statecount) {
		a_dst = 1;
	} else if (a->statecount < b->statecount) {
		a_dst = 0;
	} else {
		a_dst = 1;
	}

	if (a_dst) {
		res = merge(a, b,
		    &combine_info->base_a,
		    &combine_info->base_b,
		    &combine_info->capture_base_a,
		    &combine_info->capture_base_b);
	} else {
		res = merge(b, a,
		    &combine_info->base_b,
		    &combine_info->base_a,
		    &combine_info->capture_base_b,
		    &combine_info->capture_base_a);
	}

	return res;
}
