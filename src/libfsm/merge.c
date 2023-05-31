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
#include <adt/u64bitset.h>

#include "capture.h"
#include "capture_vm.h"
#include "internal.h"
#include "endids.h"

#define LOG_MERGE_ENDIDS 0
#define LOG_COPY_CAPTURE_PROGRAMS 0

static int
copy_end_metadata(struct fsm *dst, struct fsm *src,
    fsm_state_t base_src, unsigned capture_base_src);

static int
copy_end_ids(struct fsm *dst, struct fsm *src, fsm_state_t base_src);

static int
copy_active_capture_ids(struct fsm *dst, struct fsm *src,
    fsm_state_t base_src, unsigned capture_base_src);

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
		*capture_base_src = fsm_capture_ceiling(dst);

		for (i = 0; i < src->statecount; i++) {
			state_set_rebase(&src->states[i].epsilons, *base_src);
			edge_set_rebase(&src->states[i].edges, *base_src);
		}
	}

	memcpy(dst->states + dst->statecount, src->states,
		src->statecount * sizeof *src->states);
	dst->statecount += src->statecount;
	dst->endcount   += src->endcount;

	/* We need to explicitly copy over end metadata here. They're
	 * stored separately because they are likely to only be on a
	 * small portion of the states. */
	if (!copy_end_metadata(dst, src, *base_src, *capture_base_src)) {
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

struct copy_capture_programs_env {
	const struct fsm_alloc *alloc;
	const struct fsm *src;
	struct fsm *dst;
	int ok;
	fsm_state_t state_base_src;
	unsigned capture_base_src;

#define DEF_MAPPING_CEIL 1
	size_t mapping_used;
	size_t mapping_ceil;
	/* TODO: could cache last_map to check first if this becomes expensive */
	struct prog_mapping {
		unsigned src_prog_id;
		unsigned dst_prog_id;
	} *mappings;
};

static int
copy_capture_programs_cb(fsm_state_t src_state, unsigned src_prog_id,
    void *opaque)
{
	struct copy_capture_programs_env *env = opaque;

	const fsm_state_t dst_state = src_state + env->state_base_src;
	assert(dst_state < fsm_countstates(env->dst));

#if LOG_COPY_CAPTURE_PROGRAMS
	fprintf(stderr, "%s: src %p, dst %p, src_prog_id %u, src_state %d, dst_state %d, capture_base_src %u\n",
	    __func__, (void *)env->src, (void *)env->dst,
	    src_prog_id, src_state, dst_state, env->capture_base_src);
#endif
	int found = 0;
	uint32_t dst_prog_id;

	for (size_t i = 0; i < env->mapping_used; i++) {
		const struct prog_mapping *m = &env->mappings[i];
		if (m->src_prog_id == src_prog_id) {
			dst_prog_id = m->dst_prog_id;
			found = 1;
		}
	}

	if (!found) {
		if (env->mapping_used == env->mapping_ceil) { /* grow */
			const size_t nceil = 2*env->mapping_ceil;
			struct prog_mapping *nmappings = f_realloc(env->alloc,
			    env->mappings, nceil * sizeof(nmappings[0]));
			if (nmappings == NULL) {
				env->ok = 0;
				return 0;
			}

			env->mapping_ceil = nceil;
			env->mappings = nmappings;
		}

		const struct capvm_program *p = fsm_capture_get_program_by_id(env->src,
		    src_prog_id);
		assert(p != NULL);

		struct capvm_program *cp = capvm_program_copy(env->alloc, p);
		if (cp == NULL) {
			env->ok = 0;
			return 0;
		}
		capvm_program_rebase(cp, env->capture_base_src);

		/* add program, if not present */
		if (!fsm_capture_add_program(env->dst,
			cp, &dst_prog_id)) {
			f_free(env->alloc, cp);
			env->ok = 0;
			return 0;
		}

		struct prog_mapping *m = &env->mappings[env->mapping_used];
		m->src_prog_id = src_prog_id;
		m->dst_prog_id = dst_prog_id;
		env->mapping_used++;
	}

	/* associate with end states */
	if (!fsm_capture_associate_program_with_end_state(env->dst,
		dst_prog_id, dst_state)) {
		env->ok = 0;
		return 0;
	}

	return 1;
}

static int
copy_capture_programs(struct fsm *dst, const struct fsm *src,
	fsm_state_t state_base_src, unsigned capture_base_src)
{
	const struct fsm_alloc *alloc = src->opt->alloc;
	struct prog_mapping *mappings = f_malloc(alloc,
	    DEF_MAPPING_CEIL * sizeof(mappings[0]));
	if (mappings == NULL) {
		return 0;
	}

	struct copy_capture_programs_env env = {
		.alloc = alloc,
		.src = src,
		.dst = dst,
		.ok = 1,
		.state_base_src = state_base_src,
		.capture_base_src = capture_base_src,
		.mapping_ceil = DEF_MAPPING_CEIL,
		.mappings = mappings,
	};
	fsm_capture_iter_program_ids_for_all_end_states(src,
	    copy_capture_programs_cb, &env);

	f_free(alloc, env.mappings);

	return env.ok;
}

static int
copy_end_metadata(struct fsm *dst, struct fsm *src,
    fsm_state_t base_src, unsigned capture_base_src)
{
	/* TODO: inline */

	if (!copy_end_ids(dst, src, base_src)) {
		return 0;
	}

	if (!copy_active_capture_ids(dst, src, base_src, capture_base_src)) {
		return 0;
	}

	if (!copy_capture_programs(dst, src, base_src, capture_base_src)) {
		return 0;
	}

	return 1;
}

struct copy_end_ids_env {
	char tag;
	struct fsm *dst;
	struct fsm *src;
	fsm_state_t base_src;
	int ok;
};

static int
copy_end_ids_cb(const struct fsm *fsm, fsm_state_t state,
	size_t nth, const fsm_end_id_t id, void *opaque)
{
	enum fsm_endid_set_res sres;
	struct copy_end_ids_env *env = opaque;
	assert(env->tag == 'M');
	(void)fsm;
	(void)nth;

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
	env.base_src = base_src;
	env.ok = 1;

	fsm_endid_iter(src, copy_end_ids_cb, &env);
	return env.ok;
}

struct copy_active_capture_ids_env {
	char tag;
	struct fsm *dst;
	fsm_state_t base_src;
	unsigned capture_base_src;
	int ok;
};

static int
copy_active_capture_ids_cb(fsm_state_t state, unsigned capture_id, void *opaque)
{
	struct copy_active_capture_ids_env *env = opaque;
	assert(env->tag == 'A');

	if (!fsm_capture_set_active_for_end(env->dst,
		capture_id + env->capture_base_src,
		state + env->base_src)) {
		env->ok = 0;
		return 0;
	}
	return 1;
}

static int
copy_active_capture_ids(struct fsm *dst, struct fsm *src,
    fsm_state_t base_src, unsigned capture_base_src)
{
	struct copy_active_capture_ids_env env;
	env.tag = 'A';
	env.dst = dst;
	env.base_src = base_src;
	env.capture_base_src = capture_base_src;
	env.ok = 1;

	fsm_capture_iter_active_for_all_end_states(src,
	    copy_active_capture_ids_cb, &env);
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
