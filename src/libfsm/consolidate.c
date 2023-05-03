/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

/* fsm consolidate -> take an FSM and a mapping, produce a new FSM with combined states */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <fsm/fsm.h>
#include <fsm/capture.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <adt/alloc.h>
#include <adt/set.h>
#include <adt/edgeset.h>
#include <adt/stateset.h>

#include "internal.h"
#include "capture.h"
#include "endids.h"

#define LOG_MAPPING 0
#define LOG_CONSOLIDATE_CAPTURES 0
#define LOG_CONSOLIDATE_ENDIDS 0

struct mapping_closure {
	size_t count;
	const fsm_state_t *mapping;
};

struct consolidate_copy_capture_actions_env {
	char tag;
	struct fsm *dst;
	size_t mapping_count;
	const fsm_state_t *mapping;
	int ok;
};

static int
consolidate_copy_capture_actions(struct fsm *dst, const struct fsm *src,
    const fsm_state_t *mapping, size_t mapping_count);

static int
consolidate_end_ids(struct fsm *dst, const struct fsm *src,
    const fsm_state_t *mapping, size_t mapping_count);

static fsm_state_t
mapping_cb(fsm_state_t id, const void *opaque)
{
	const struct mapping_closure *closure = opaque;
	assert(id < closure->count);
	return closure->mapping[id];
}

struct fsm *
fsm_consolidate(const struct fsm *src,
    const fsm_state_t *mapping, size_t mapping_count)
{
	struct fsm *dst;
	fsm_state_t src_i;
	uint64_t *seen = NULL;
	struct mapping_closure closure;
	size_t max_used = 0;

	assert(src != NULL);
	assert(src->opt != NULL);

	dst = fsm_new(src->opt);
	if (dst == NULL) {
		goto cleanup;
	}

	for (src_i = 0; src_i < mapping_count; src_i++) {
#if LOG_MAPPING
		fprintf(stderr, "consolidate_mapping[%u]: %u\n",
		    src_i, mapping[src_i]);
#endif
		if (mapping[src_i] >= max_used) {
			max_used = mapping[src_i];
		}
	}

	if (!fsm_addstate_bulk(dst, max_used + 1)) {
		goto cleanup;
	}
	assert(dst->statecount == max_used + 1);

	seen = f_calloc(src->opt->alloc,
	    mapping_count/64 + 1, sizeof(seen[0]));
	if (seen == NULL) {
		goto cleanup;
	}

#define DST_SEEN(I) (seen[I/64] & ((uint64_t)1 << (I&63)))
#define SET_DST_SEEN(I) (seen[I/64] |= ((uint64_t)1 << (I&63)))

	/* map N states to M states, where N >= M.
	 * if it's the first time state[M] is seen,
	 * then copy it verbatim, otherwise only
	 * carryopaque. */

	closure.count = mapping_count;
	closure.mapping = mapping;

	for (src_i = 0; src_i < mapping_count; src_i++) {
		const fsm_state_t dst_i = mapping[src_i];

		if (!DST_SEEN(dst_i)) {
			SET_DST_SEEN(dst_i);

			if (!state_set_copy(&dst->states[dst_i].epsilons,
				dst->opt->alloc, src->states[src_i].epsilons)) {
				goto cleanup;
			}
			state_set_compact(&dst->states[dst_i].epsilons,
			    mapping_cb, &closure);

			if (!edge_set_copy(&dst->states[dst_i].edges,
				dst->opt->alloc,
				src->states[src_i].edges)) {
				goto cleanup;
			}
			edge_set_compact(&dst->states[dst_i].edges,
			    dst->opt->alloc, mapping_cb, &closure);

			if (fsm_isend(src, src_i)) {
				fsm_setend(dst, dst_i, 1);
			}
		}
	}

	if (!consolidate_copy_capture_actions(dst, src, mapping, mapping_count)) {
		goto cleanup;
	}

	if (!consolidate_end_ids(dst, src, mapping, mapping_count)) {
		goto cleanup;
	}

	{
		fsm_state_t src_start, dst_start;
		if (fsm_getstart(src, &src_start)) {
			assert(src_start < mapping_count);
			dst_start = mapping[src_start];
			fsm_setstart(dst, dst_start);
		}
	}

	f_free(src->opt->alloc, seen);

	return dst;

cleanup:

	if (seen != NULL) { f_free(src->opt->alloc, seen); }
	return NULL;
}

static int
consolidate_copy_capture_actions_cb(fsm_state_t state,
    enum capture_action_type type, unsigned capture_id, fsm_state_t to,
    void *opaque)
{
	struct consolidate_copy_capture_actions_env *env = opaque;
	fsm_state_t s, t;

	assert(env->tag == 'C');

#if LOG_CONSOLIDATE_CAPTURES
	fprintf(stderr, "consolidate_copy_capture_actions_cb: state %u, type %s, ID %u, TO %d\n",
	    state,
	    fsm_capture_action_type_name[type],
	    capture_id, to);
#endif

	assert(state < env->mapping_count);
	assert(to == CAPTURE_NO_STATE || to < env->mapping_count);
	s = env->mapping[state];
	t = to == CAPTURE_NO_STATE
	    ? CAPTURE_NO_STATE : env->mapping[to];

	if (!fsm_capture_add_action(env->dst,
		s, type, capture_id, t)) {
		env->ok = 0;
		return 0;
	}

	return 1;
}

static int
consolidate_copy_capture_actions(struct fsm *dst, const struct fsm *src,
    const fsm_state_t *mapping, size_t mapping_count)
{
	size_t i;

	struct consolidate_copy_capture_actions_env env;
	env.tag = 'C';
	env.dst = dst;
	env.mapping_count = mapping_count;
	env.mapping = mapping;
	env.ok = 1;

#if LOG_MAPPING
	for (i = 0; i < mapping_count; i++) {
		fprintf(stderr, "mapping[%lu]: %u\n", i, mapping[i]);
	}
#else
	(void)i;
#endif

	fsm_capture_action_iter(src,
	    consolidate_copy_capture_actions_cb, &env);
	return env.ok;
}

struct consolidate_end_ids_env {
	char tag;
	struct fsm *dst;
	const struct fsm *src;
	const fsm_state_t *mapping;
	size_t mapping_count;
};

static int
consolidate_end_ids_cb(fsm_state_t state, const fsm_end_id_t *ids, size_t num_ids, void *opaque)
{
	struct consolidate_end_ids_env *env = opaque;
	enum fsm_endid_set_res sres;
	fsm_state_t s;
	assert(env->tag == 'C');

	assert(state < env->mapping_count);
	s = env->mapping[state];

	sres = fsm_endid_set_bulk(env->dst, s, num_ids, ids, FSM_ENDID_BULK_APPEND);
	if (sres == FSM_ENDID_SET_ERROR_ALLOC_FAIL) {
		return 0;
	}

	return 1;
}

static int
consolidate_end_ids(struct fsm *dst, const struct fsm *src,
    const fsm_state_t *mapping, size_t mapping_count)
{
	struct consolidate_end_ids_env env;
	int ret;

	env.tag = 'C';		/* for Consolidate */
	env.dst = dst;
	env.src = src;
	env.mapping = mapping;
	env.mapping_count = mapping_count;

	ret = fsm_endid_iter_bulk(src, consolidate_end_ids_cb, &env);

#if LOG_CONSOLIDATE_ENDIDS > 1
	fprintf(stderr, "==== fsm_consolidate -- endid_info after:\n");
	fsm_endid_dump(stderr, dst);
#endif

	return ret;
}
