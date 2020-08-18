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
#include <fsm/pred.h>
#include <fsm/walk.h>

#include <adt/alloc.h>
#include <adt/set.h>
#include <adt/edgeset.h>
#include <adt/stateset.h>
#include <adt/hashset.h>
#include <adt/mappinghashset.h>

#include "internal.h"

#define LOG_MAPPING 0

struct mapping_closure {
	size_t count;
	const fsm_state_t *mapping;
};

static fsm_state_t
mapping_cb(fsm_state_t id, const void *opaque)
{
	const struct mapping_closure *closure = opaque;
	assert(id < closure->count);
	return closure->mapping[id];
}

struct fsm *
fsm_consolidate(struct fsm *src,
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

			dst->states[dst_i].opaque = src->states[src_i].opaque;

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
			    mapping_cb, &closure);

			if (fsm_isend(src, src_i)) {
				fsm_setend(dst, dst_i, 1);
			}
		} else {
			const int is_end = fsm_isend(dst, dst_i);
			fsm_state_t prev_i;
			fsm_state_t prev_src_id = src->statecount; /* NONE */
			fsm_state_t src_set[2];

			if (!is_end) {
				continue;
			}
			assert(fsm_isend(src, src_i));

			/* Find the previous state that also maps to this
			 * state, to carry the opaque from it. This could
			 * become expensive when there are a very high number
			 * of states, so it may be worth keeping a temporary
			 * second mapping for just the end states. */
			for (prev_i = 0; prev_i < mapping_count; prev_i++) {
				if (mapping[prev_i] == dst_i) {
					prev_src_id = prev_i;
					break;
				}
			}
			assert(prev_src_id < src->statecount);

			src_set[0] = prev_src_id;
			src_set[1] = src_i;

			/* call carryopaque pairwise */
			fsm_carryopaque_array(src,
			    src_set, 2,
			    dst, dst_i);
		}
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
