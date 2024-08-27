/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/capture.h>
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/options.h>
#include <fsm/print.h>

#include "internal.h"

#include <adt/edgeset.h>
#include "eager_output.h"

#define LOG_UNION_ARRAY 0

struct fsm *
fsm_union(struct fsm *a, struct fsm *b,
	struct fsm_combine_info *combine_info)
{
	struct fsm *q;
	fsm_state_t sa, sb;
	fsm_state_t sq;
	struct fsm_combine_info combine_info_internal;

	if (combine_info == NULL) {
		combine_info = &combine_info_internal;
	}

	memset(combine_info, 0x00, sizeof(*combine_info));

	assert(a != NULL);
	assert(b != NULL);

	if (a->alloc != b->alloc) {
		errno = EINVAL;
		return NULL;
	}

	if (a->statecount == 0) { fsm_free(a); return b; }
	if (b->statecount == 0) { fsm_free(b); return a; }

	if (!fsm_getstart(a, &sa) || !fsm_getstart(b, &sb)) {
		errno = EINVAL;
		return NULL;
	}

	q = fsm_merge(a, b, combine_info);
	assert(q != NULL);

	sa += combine_info->base_a;
	sb += combine_info->base_b;

	/*
	 * The textbook approach is to create a new start state, with epsilon
	 * transitions to both a->start and b->start:
	 *
	 *     a: ⟶ ○ ··· ◎             ╭⟶ ○ ··· ◎
	 *                     a ∪ b: ⟶ ○
	 *     b: ⟶ ○ ··· ◎             ╰⟶ ○ ··· ◎
	 */

	if (!fsm_addstate(q, &sq)) {
		goto error;
	}

	fsm_setstart(q, sq);

	if (sq != sa) {
		if (!fsm_addedge_epsilon(q, sq, sa)) {
			goto error;
		}
	}

	if (sq != sb) {
		if (!fsm_addedge_epsilon(q, sq, sb)) {
			goto error;
		}
	}

	return q;

error:

	fsm_free(q);

	return NULL;
}

struct fsm *
fsm_union_array(size_t fsm_count,
    struct fsm **fsms, struct fsm_combined_base_pair *bases)
{
	size_t i;
	struct fsm *res = fsms[0];

	fsms[0] = NULL;
	if (bases != NULL) {
		memset(bases, 0x00, fsm_count * sizeof(bases[0]));
	}

	for (i = 1; i < fsm_count; i++) {
		struct fsm_combine_info ci;
		struct fsm *combined = fsm_union(res, fsms[i], &ci);
		fsms[i] = NULL;
		if (combined == NULL) {
			while (i < fsm_count) {
				fsm_free(fsms[i]);
				i++;
			}

			fsm_free(res);
			return NULL;
		}

		res = combined;

		if (bases == NULL) {
			continue;
		}

		bases[i].state = ci.base_b;
		bases[i].capture = ci.capture_base_b;

		/* If the first argument didn't get its states put first
		 * in the union, then shift the bases for everything
		 * that has been combined into it so far. */
		if (ci.capture_base_a > 0) {
			size_t shift_i;
			for (shift_i = 0; shift_i < i; shift_i++) {
				bases[shift_i].state += ci.base_a;
				bases[shift_i].capture += ci.capture_base_a;
			}
		}
	}

#if LOG_UNION_ARRAY
	if (bases != NULL) {
		for (i = 0; i < fsm_count; i++) {
			fprintf(stderr, "union_array: bases %u: %zu, %zu\n",
				i, bases[i].state, bases[i].capture);
		}
	}
#endif

	return res;
}

#define LOG_UNION_REPEATED_PATTERN_GROUP 0

/* Combine an array of FSMs into a single FSM in one pass, with an extra loop
 * so that more than one pattern with eager outputs can match. */
struct fsm *
fsm_union_repeated_pattern_group(size_t entry_count,
    struct fsm_union_entry *entries, struct fsm_combined_base_pair *bases)
{
	const struct fsm_alloc *alloc = entries[0].fsm->alloc;
	const bool log = 0 || LOG_UNION_REPEATED_PATTERN_GROUP;

	if (entry_count == 1) {
		return entries[0].fsm;
	}

	size_t est_total_states = 0;
	for (size_t i = 0; i < entry_count; i++) {
		assert(entries[i].fsm);
		if (entries[i].fsm->alloc != alloc) {
			errno = EINVAL;
			return NULL;
		}
		const size_t count = fsm_countstates(entries[i].fsm);
		est_total_states += count;
	}

	est_total_states += 5;	/* new start and end, new unanchored start and end loops */

	struct fsm *res = fsm_new_statealloc(alloc, est_total_states);
	if (res == NULL) { return NULL; }

	/* collected end states */
	struct ends_buf {
		size_t ceil;
		size_t used;
		fsm_state_t *states;
	} ends = { .ceil = 0 };

	/* The new overall start state, which will have an epsilon edge to... */
	fsm_state_t global_start;
	if (!fsm_addstate(res, &global_start)) { goto fail; }

	/* states linking to the starts of unanchored and anchored subgraphs, respectively. */
	fsm_state_t global_start_loop, global_start_anchored;
	if (!fsm_addstate(res, &global_start_loop)) { goto fail; }
	if (!fsm_addstate(res, &global_start_anchored)) { goto fail; }

	/* The unanchored end loop state, and an end state with no outgoing edges. */
	fsm_state_t global_end_loop, global_end;
	if (!fsm_addstate(res, &global_end)) { goto fail; }
	if (!fsm_addstate(res, &global_end_loop)) { goto fail; }

	/* link the start to the start loop and anchored start, and the start loop to itself */
	if (log) {
		fprintf(stderr, "link_before: global_start %d -> global_start_loop %d and global_start_anchored %d\n",
		    global_start, global_start_loop, global_start_anchored);
	}
	if (!fsm_addedge_epsilon(res, global_start, global_start_loop)) { goto fail; }
	if (!fsm_addedge_epsilon(res, global_start, global_start_anchored)) { goto fail; }
	if (!fsm_addedge_any(res, global_start_loop, global_start_loop)) { goto fail; }

	/* link the end loop and end */
	if (log) {
		fprintf(stderr, "link_before: global_end_loop %d -> global_end %d (and -> self)\n", global_end_loop, global_end);
	}
	if (!fsm_addedge_epsilon(res, global_end_loop, global_end)) { goto fail; }
	if (!fsm_addedge_any(res, global_end_loop, global_end_loop)) { goto fail; }

	if (bases != NULL) {
		memset(bases, 0x00, entry_count * sizeof(bases[0]));
	}

	for (size_t fsm_i = 0; fsm_i < entry_count; fsm_i++) {
		ends.used = 0;	/* reset */

		struct fsm *fsm = entries[fsm_i].fsm;
		entries[fsm_i].fsm = NULL; /* transfer ownership */

		const size_t state_count = fsm_countstates(fsm);

		fsm_state_t fsm_start;
		if (!fsm_getstart(fsm, &fsm_start)) {
			fsm_free(fsm);		      /* no start, just discard */
			continue;
		}

		for (fsm_state_t s_i = 0; s_i < state_count; s_i++) {
			if (fsm_isend(fsm, s_i)) {
				if (ends.used == ends.ceil) { /* grow? */
					size_t nceil = (ends.ceil == 0 ? 4 : 2*ends.ceil);
					fsm_state_t *nstates = f_realloc(alloc,
					    ends.states, nceil * sizeof(nstates[0]));
					if (nstates == NULL) { goto fail; }
					ends.ceil = nceil;
					ends.states = nstates;
				}
				ends.states[ends.used++] = s_i;
			}
		}

		if (ends.used == 0) {
			fsm_free(fsm);		      /* no ends, just discard */
			continue;
		}

		/* When combining these, remove self-edges from any states on the FSMs to be
		 * combined that also have eager output IDs. We are about to add an epsilon edge
		 * from each to a shared state that won't have eager output IDs.
		 *
		 * Eager output matching should be idempotent, so carrying it to other reachable
		 * state is redundant, and it leads to a combinatorial explosion that blows up the
		 * state count while determinising the combined FSM otherwise.
		 *
		 * For example, if /aaa/, /bbb/, and /ccc/ are combined into a DFA that repeats
		 * the sub-patterns (like `^.*(?:(aaa)|(bbb)|(ccc))+.*$`), the self-edge at each
		 * eager output state would combine with every reachable state from then on,
		 * leading to a copy of the whole reachable subgraph colored by every
		 * combination of eager output IDs: aaa, bbb, ccc, aaa+bbb, aaa+ccc,
		 * bbb+ccc, aaa+bbb+ccc. Instead of three relatively separate subgraphs
		 * that set the eager output at their last state, one for each pattern,
		 * it leads to 8 (2**3) subgraph clusters because it encodes _each
		 * distinct combination_ in the DFA. This becomes incredibly expensive
		 * as the combined pattern count increases; it's essentially what I'm
		 * trying to avoid by adding eager output support in the first place.
		 *
		 * FIXME: instead of actively removing these, filter in fsm_determinise? */
		if (fsm_eager_output_has_eager_output(fsm)) {
			/* for any state that has eager outputs and a self edge,
			 * remove the self edge before further linkage */
			for (fsm_state_t s = 0; s < fsm->statecount; s++) {
				if (!fsm_eager_output_has_any(fsm, s, NULL)) { continue; }
				struct edge_set *edges = fsm->states[s].edges;
				struct edge_set *new = edge_set_new();

				struct edge_group_iter iter;
				struct edge_group_iter_info info;
				edge_set_group_iter_reset(edges, EDGE_GROUP_ITER_ALL, &iter);
				while (edge_set_group_iter_next(&iter, &info)) {
					if (info.to != s) {
						if (!edge_set_add_bulk(&new, fsm->alloc,
							info.symbols, info.to)) {
							goto fail;
						}
					}
				}
				edge_set_free(fsm->alloc, edges);
				fsm->states[s].edges = new;
			}
		}

		/* call fsm_merge; we really don't care which is which */
		struct fsm_combine_info combine_info;
		struct fsm *merged = fsm_merge(res, fsm, &combine_info);
		if (merged == NULL) { goto fail; }

		/* update offsets if res had its state IDs shifted forward */
		global_start += combine_info.base_a;
		global_start_loop += combine_info.base_a;
		global_start_anchored += combine_info.base_a;;
		global_end += combine_info.base_a;
		global_end_loop += combine_info.base_a;

		/* also update offsets for the FSM's states */
		fsm_start += combine_info.base_b;
		for (size_t i = 0; i < ends.used; i++) {
			ends.states[i] += combine_info.base_b;
		}

		if (bases != NULL) {
			bases[fsm_i].state = combine_info.base_b;
			bases[fsm_i].capture = combine_info.capture_base_b;
		}

		if (log) {
			fprintf(stderr, "%s: fsm[%zd].start: %d\n", __func__, fsm_i, fsm_start);
			for (size_t i = 0; i < ends.used; i++) {
				fprintf(stderr, "%s: fsm[%zd].ends[%zd]: %d\n", __func__, fsm_i, i, ends.states[i]);
			}
		}

		/* link to the FSM's start state */
		const fsm_state_t start_src = entries[fsm_i].anchored_start ? global_start_anchored : global_start_loop;
		if (!fsm_addedge_epsilon(merged, start_src, fsm_start)) { goto fail; }
		if (log) {
			fprintf(stderr, "%s: linking %s %d to fsm[%zd]'s start %d (anchored? %d)\n",
			    __func__,
			    entries[fsm_i].anchored_start ? "global_start_anchored" : "global_start_loop",
			    start_src, fsm_i, fsm_start, entries[fsm_i].anchored_start);
		}

		/* link from the FSM's ends */
		const fsm_state_t end_dst = entries[fsm_i].anchored_end ? global_end : global_end_loop;
		for (size_t i = 0; i < ends.used; i++) {
			if (log) {
				fprintf(stderr, "%s: linking fsm[%zd]'s end[%zd] %d (anchored? %d) to %s %d\n",
				    __func__, fsm_i, i, ends.states[i], entries[fsm_i].anchored_end,
				    entries[fsm_i].anchored_end ? "global_end" : "global_end_loop",
				    end_dst);
			}
			if (!fsm_addedge_epsilon(merged, ends.states[i], end_dst)) { goto fail; }
		}

		res = merged;
	}

	/* Link from the global_end_loop to the global_start_loop, so patterns with an
	 * unanchored start can follow other patterns with an unanchored end. */
	if (log) {
		fprintf(stderr, "%s: g_start %d, g_start_loop %d, g_start_anchored %d, g_end_loop %d, g_end %d (after all merging)\n",
		    __func__, global_start, global_start_loop, global_start_anchored, global_end_loop, global_end);
		fprintf(stderr, "%s: linking global_end_loop %d to global_start_loop %d\n",
		    __func__, global_end_loop, global_start_loop);
		fprintf(stderr, "%s: setting global_start %d and end %d\n", __func__, global_start, global_end);
	}
	if (!fsm_addedge_epsilon(res, global_end_loop, global_start_loop)) { goto fail; }

	/* This needs to be set after merging, because that clears the start state. */
	fsm_setstart(res, global_start);
	fsm_setend(res, global_end, 1);

	f_free(alloc, ends.states);
	return res;

fail:
	f_free(alloc, ends.states);
	return NULL;
}
