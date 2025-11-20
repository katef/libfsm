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
#include <adt/stateset.h>

#include "eager_output.h"
#include "endids.h"

#define LOG_UNION_ARRAY 0
#define LOG_ANALYZE_GROUP_NFA 0
#define LOG_AFTER_MODIFY_GROUP_NFA 0
#define LOG_ANALYZE_GROUP_NFA_RESULTS (0 || LOG_ANALYZE_GROUP_NFA > 1)
#define LOG_UNION_REPEATED_PATTERN_GROUP 0
#define LOG_FSM_UNION_REPEATED_PATTERN_GROUP_OUTPUT 0

#define NO_STATE (fsm_state_t)-1

/* State/edge info gathered about an NFA. Used by fsm_union_repeated_pattern_group. */
struct analysis_info {
	bool nullable;          /* Does the NFA match the empty string? */
	fsm_state_t start;	/* start state */

	/* The states with a /./ self edge representing the unanchored
	 * start and end, or LINKAGE_NO_STATE. There can be at most one
	 * of each. Copied from linkage_info. */
	fsm_state_t unanchored_start_loop;
	fsm_state_t unanchored_end_loop;

	/* The end state following the unanchored end loop.
	 * Copied from linkage_info.*/
	fsm_state_t unanchored_end_loop_end;

	/* States that link to paths only reachable from the beginning of input.
	 * Copied from linkage_info. */
	struct state_set *anchored_starts;
	/* States leading to an anchored end. Copied from linkage_info. */
	struct state_set *anchored_ends;

	/* States with an outgoing labeled edge to the unanchored end loop. Input
	 * following those edges has matched, but may still consume trailing input.
	 * These edges correspond to edges leaving capture group 0 in PCRE. */
	struct state_set *eager_matches;

	/* Edges leading to states that can only match at the start of input. */
	struct edge_set *anchored_firsts;

	/* Edges leading to states that can begin an unanchored match,
	 * potentially after other combined patterns have matched. */
	struct edge_set *repeatable_firsts;

	/* A new state that may be added while replacing the unanchored_end_loop,
	 * if present. This state exists to set an eager output ID and have an
	 * epsilon edge to the combined NFA's global_unanchored_end_loop.
	 * The old unanchored_end_loop will be disconnected. */
	fsm_state_t eager_match_state;

	/* These states need an epsilon edge added to the eager_matched_state. */
	struct state_set *needs_indirect_epsilon_edge_to_eager_match_state;

	/* States which are reachable from any state besides the start
	 * state. This can be necessary to correctly identify the
	 * unanchored start loop. */
	struct state_set *reachable_from_nonstart_state;
};

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

#if LOG_ANALYZE_GROUP_NFA_RESULTS
static void
dump_state_set(FILE *f, const char *name, const struct state_set *set)
{
	struct state_iter si;
	fsm_state_t s;
	if (state_set_empty(set)) { return; }

	fprintf(f, "  - %s:", name);
	state_set_reset(set, &si);
	while (state_set_next(&si, &s)) {
		fprintf(f, " %d", s);
	}
	fprintf(f, "\n");
}

static void
dump_edge_set(FILE *f, const char *name, fsm_state_t from, const struct edge_set *edges)
{
	struct edge_group_iter iter;
	struct edge_group_iter_info info;
	if (edge_set_empty(edges)) { return; }

	fprintf(f, "  - %s:", name);
	edge_set_group_iter_reset(edges, EDGE_GROUP_ITER_ALL, &iter);
	while (edge_set_group_iter_next(&iter, &info)) {
		fprintf(f, " %d->%d", from, info.to);
	}
	fprintf(f, "\n");
}
#endif

static bool
state_has_labeled_edge_to_eclosure_with_unanchored_end_loop(const struct fsm *nfa,
    fsm_state_t s_i, struct state_set **eclosures,
    fsm_state_t unanchored_end_loop,
    fsm_state_t *indirect_dst)
{
	if (unanchored_end_loop == NO_STATE) { return false; }
	assert(unanchored_end_loop < nfa->statecount);

	assert(s_i < nfa->statecount);

	/* The unanchored_end_loop doesn't count, here. */
	if (s_i == unanchored_end_loop) { return false; }

	/* Check whether the state has a labeled edge to a state with the
	 * unanchored_end_loop in its epsilon closure. */
	const struct fsm_state *s = &nfa->states[s_i];
	struct edge_group_iter iter;
	struct edge_group_iter_info info;
	edge_set_group_iter_reset(s->edges, EDGE_GROUP_ITER_ALL, &iter);
	while (edge_set_group_iter_next(&iter, &info)) {
		assert(info.to < nfa->statecount);
		const struct state_set *to_eclosure = eclosures[info.to];

		struct state_iter dst_si;
		state_set_reset(to_eclosure, &dst_si);
		fsm_state_t dst_s_i;
		while (state_set_next(&dst_si, &dst_s_i)) {
			if (dst_s_i == unanchored_end_loop) {
				if (info.to != unanchored_end_loop) {
					*indirect_dst = info.to;
				}

				return true;
			}
		}
	}

	return false;
}

static bool
start_state_epsilon_closure_matches_empty_string(const struct fsm *nfa, const struct state_set *eclosure)
{
	struct state_iter si;
	state_set_reset(eclosure, &si);

	fsm_state_t s_i;
	while (state_set_next(&si, &s_i)) {
		if (fsm_isend(nfa, s_i)) { return true; }
	}

	return false;
}

static const struct fsm_options dump_nfa_opt = {
	.io = FSM_IO_STR,
	.ambig = AMBIG_MULTIPLE,
	.case_ranges = 1,
	.consolidate_edges = 1,
	.group_edges = 1,
};

static bool
analyze_group_nfa(const struct fsm *nfa, struct analysis_info *ainfo)
{
	if (LOG_ANALYZE_GROUP_NFA) {
		fprintf(stderr, "==== %s\n", __func__);
		if (LOG_ANALYZE_GROUP_NFA > 1) {
			fsm_print(stderr, nfa, &dump_nfa_opt, NULL, FSM_PRINT_DOT);
		}
	}

	ainfo->start = NO_STATE;
	ainfo->eager_match_state = NO_STATE;

	if (!fsm_getstart(nfa, &ainfo->start)) {
		return false;
	}

	const size_t state_count = fsm_countstates(nfa);
	assert(ainfo->start < state_count);

	struct state_set **eclosures = epsilon_closure(nfa);
	if (eclosures == NULL) {
		return false;
	}

	/* Mark any states that are reachable from any state besides the
	 * start state -- this means they cannot be the unanchored start
	 * loop, in cases where the pass below would otherwise detect
	 * more than one. */
	for (fsm_state_t s_i = 0; s_i < state_count; s_i++) {
		if (s_i == ainfo->start) { continue; }

		struct state_iter si;
		state_set_reset(nfa->states[s_i].epsilons, &si);
		fsm_state_t eps_i;
		while (state_set_next(&si, &eps_i)) {
			/* Ignore self edges */
			if (eps_i == s_i) { continue; }

			if (!state_set_add(&ainfo->reachable_from_nonstart_state, nfa->alloc, eps_i)) {
				return false;
			}
		}

		struct edge_group_iter egi;
		struct edge_group_iter_info info;
		edge_set_group_iter_reset(nfa->states[s_i].edges, EDGE_GROUP_ITER_ALL, &egi);
		while (edge_set_group_iter_next(&egi, &info)) {
			/* Ignore self edges */
			if (info.to == s_i) { continue; }

			if (!state_set_add(&ainfo->reachable_from_nonstart_state, nfa->alloc, info.to)) {
				return false;
			}
		}
	}

	/* Copy labeled edges from the unanchored start loop and
	 * its epsilon closure to ainfo->repeatable_firsts, except
	 * for edges leading back to the unanchored start loop. */
	if (ainfo->unanchored_start_loop != NO_STATE) {
		struct state_iter si;
		state_set_reset(eclosures[ainfo->unanchored_start_loop], &si);
		fsm_state_t cs_i;
		while (state_set_next(&si, &cs_i)) {
			assert(cs_i < nfa->statecount);
			const struct fsm_state *cs = &nfa->states[cs_i]; /* closure state */

			struct edge_group_iter egi;
			struct edge_group_iter_info info;

			edge_set_group_iter_reset(cs->edges, EDGE_GROUP_ITER_ALL, &egi);
			while (edge_set_group_iter_next(&egi, &info)) {
				if (info.to != ainfo->unanchored_start_loop) {
					if (!edge_set_add_bulk(&ainfo->repeatable_firsts,
						nfa->alloc, info.symbols, info.to)) {
						goto alloc_fail;
					}
				}
			}
		}
	}

	/* Copy labeled edges from the anchored start and its epsilon
	 * closure to ainfo->anchored_firsts. */
	struct state_iter si_anchored_start;
	state_set_reset(ainfo->anchored_starts, &si_anchored_start);
	fsm_state_t anchored_start;
	while (state_set_next(&si_anchored_start, &anchored_start)) {
		struct state_iter si;
		state_set_reset(eclosures[anchored_start], &si);
		fsm_state_t cs_i;
		while (state_set_next(&si, &cs_i)) {
			assert(cs_i < nfa->statecount);
			const struct fsm_state *cs = &nfa->states[cs_i];

			struct edge_group_iter egi;
			struct edge_group_iter_info info;

			edge_set_group_iter_reset(cs->edges, EDGE_GROUP_ITER_ALL, &egi);
			while (edge_set_group_iter_next(&egi, &info)) {
				if (!edge_set_add_bulk(&ainfo->anchored_firsts,
					nfa->alloc, info.symbols, info.to)) {
					goto alloc_fail;
				}
			}
		}
	}

	/* If the start state always matches, set a flag noting that it will need special handling
	 * later. It's arguably pointless to combine "" with other regexes, because it will always
	 * trivially match, but otherwise it would never match. */
	ainfo->nullable = start_state_epsilon_closure_matches_empty_string(nfa, eclosures[ainfo->start]);

	/* Collect states that lead to an anchored end or eager match. */
	for (size_t s_i = 0; s_i < state_count; s_i++) {
		fsm_state_t indirect_dst = NO_STATE;
		if (state_has_labeled_edge_to_eclosure_with_unanchored_end_loop(nfa, s_i, eclosures, ainfo->unanchored_end_loop, &indirect_dst)) {
			if (!state_set_add(&ainfo->eager_matches, nfa->alloc, s_i)) {
				goto alloc_fail;
			}
		}

		if (indirect_dst != NO_STATE) {
			if (!state_set_add(&ainfo->needs_indirect_epsilon_edge_to_eager_match_state, nfa->alloc, indirect_dst)) {
				goto alloc_fail;
			}
		}
	}

#if LOG_ANALYZE_GROUP_NFA_RESULTS
	{
		fprintf(stderr, "# analysis_info start %d, usl %d, uel %d, uele %d\n",
		    ainfo->start, ainfo->unanchored_start_loop, ainfo->unanchored_end_loop, ainfo->unanchored_end_loop_end);
		dump_state_set(stderr, "anchored_ends", ainfo->anchored_ends);
		dump_state_set(stderr, "eager_matches", ainfo->eager_matches);
		dump_edge_set(stderr, "repeatable_firsts", ainfo->unanchored_start_loop, ainfo->repeatable_firsts);
	}
#endif

	closure_free(nfa, eclosures, state_count);

	/* The unanchored start and end loop cannot be the same state. */
	assert(ainfo->unanchored_start_loop == NO_STATE
	    || ainfo->unanchored_start_loop != ainfo->unanchored_end_loop);

	return true;

alloc_fail:
	fprintf(stderr, "alloc fail\n");
	if (eclosures != NULL) {
		closure_free(nfa, eclosures, state_count);
	}

	return false;
}

/* Replace any labeled edges on nfa->states[from_state] going to old_to
 * with a new edge leading to new_to. There currently isn't a function in
 * the libfsm API for this (and it shouldn't be necessary in general), but
 * if it gets one later this can be replaced. */
static bool
replace_labeled_edge(struct fsm *nfa, fsm_state_t from_state, fsm_state_t old_to, fsm_state_t new_to,
    bool *found)
{
	if (old_to == NO_STATE) {
		/* nothing to do */
		return true;
	}
	assert(new_to < nfa->statecount);
	assert(from_state < nfa->statecount);

	struct fsm_state *from = &nfa->states[from_state];
	struct edge_set *old_edges = from->edges;
	struct edge_set *new_edges = edge_set_new();

	/* copy, replacing edges to old_to */
	struct edge_group_iter iter;
	struct edge_group_iter_info info;
	edge_set_group_iter_reset(old_edges, EDGE_GROUP_ITER_ALL, &iter);
	while (edge_set_group_iter_next(&iter, &info)) {
		if (info.to == old_to) { *found = true; }
		if (!edge_set_add_bulk(&new_edges, nfa->alloc, info.symbols,
			info.to == old_to ? new_to : info.to)) {
			return false;
		}
	}

	edge_set_free(nfa->alloc, old_edges);
	from->edges = new_edges;
	return true;
}

/* Make a couple changes to the group NFA so that it can be combined correctly:
 *
 * - If the group NFA has an unanchored_end_loop, add a new state,
 *   eager_match_state, which will be a waypoint between edges that previously
 *   led to the unanchored_end_loop and the global NFA's global_unanchored_end_loop
 *   (so it can potentially also match other group NFAs with unanchored starts).
 *   This state will get an eager output ID.
 *
 * - Set an end ID on every anchored end state, so halting on these counts as a match. */
static bool
modify_group_nfa(struct fsm *nfa, size_t id, struct analysis_info *ainfo, size_t id_base)
{
	const bool nullable_and_unanchored_end = ainfo->nullable
	    && ainfo->unanchored_end_loop != NO_STATE;
	const bool log = 0 || (LOG_ANALYZE_GROUP_NFA > 0);

	/* Add the eager match state if there are eager match states
	 * or a nullable unanchored end. This will link to the global NFA's
	 * unanchored_end_loop. */
	if (!state_set_empty(ainfo->eager_matches) || nullable_and_unanchored_end) {
		if (!fsm_addstate(nfa, &ainfo->eager_match_state)) {
			return false;
		}
		if (log) {
			fprintf(stderr, "%s: added eager_match_state %d\n", __func__, ainfo->eager_match_state);
		}

		/* Set eager match ID on new eager_match_state. */
		const fsm_output_id_t oid = (fsm_output_id_t)(id + id_base);
		if (!fsm_eager_output_set(nfa, ainfo->eager_match_state, oid)) {
			return false;
		}
		if (log) {
			fprintf(stderr, "%s: set eager_output id %d on eager_match_state %d\n",
			    __func__, oid, ainfo->eager_match_state);
		}

		/* For every state in eager_matches, replace every edge leading to
		 * the unanchored_end_loop with an edge with the same labels to
		 * eager_match_state.
		 *
		 * If the labeled edge does not directly lead to the unanchored_end_loop,
		 * then add an epsilon edge from wherever it leads to eager_match_state
		 * instead. */
		struct state_iter si;
		state_set_reset(ainfo->eager_matches, &si);
		fsm_state_t ems_i;
		while (state_set_next(&si, &ems_i)) {
			bool found = false;
			if (!replace_labeled_edge(nfa, ems_i,
				ainfo->unanchored_end_loop, ainfo->eager_match_state, &found)) {
				return false;
			}

			if (log) {
				if (found) {
					fprintf(stderr, "%s: replacing labeled edges from eager_match_state %d to unanchored_end_loop %d with edge to new eager_match_state %d\n",
					    __func__, ems_i, ainfo->unanchored_end_loop, ainfo->eager_match_state);
				} else if (!found && ainfo->unanchored_end_loop != NO_STATE) {
					fprintf(stderr, "%s: not found: labeled edges from eager_match_state %d to unanchored_end_loop %d\n",
					    __func__, ems_i, ainfo->unanchored_end_loop);
				}
			}

			/* The state must not link to the unanchored end loop anymore.
			 * Doing so will cause a combinatorial explosion that makes
			 * combining more ~10 NFAs incredibly expensive. */
			struct edge_group_iter iter;
			struct edge_group_iter_info info;
			assert(ems_i < nfa->statecount);
			const struct fsm_state *ems = &nfa->states[ems_i];
			edge_set_group_iter_reset(ems->edges, EDGE_GROUP_ITER_ALL, &iter);
			while (edge_set_group_iter_next(&iter, &info)) {
				assert(info.to != ainfo->unanchored_end_loop);
			}
		}

		state_set_reset(ainfo->needs_indirect_epsilon_edge_to_eager_match_state, &si);
		fsm_state_t intermediate_i;
		while (state_set_next(&si, &intermediate_i)) {
			if (!fsm_addedge_epsilon(nfa, intermediate_i, ainfo->eager_match_state)) { return false; }
			if (log) {
				fprintf(stderr, "%s: adding epsilon edge from intermediate eager match state %d to new eager_match_state %d\n",
				    __func__, intermediate_i, ainfo->eager_match_state);
			}
		}
	}

	/* If the group NFA matches the empty string and has an unanchored end, then
	 * link its unanchored start state to the eager match state. This ensures
	 * all inputs will match this group NFA when combined. */
	if (nullable_and_unanchored_end) {
		assert(ainfo->start != NO_STATE);
		assert(ainfo->eager_match_state != NO_STATE);

		if (ainfo->unanchored_start_loop != NO_STATE) {
			struct fsm_state *s = &nfa->states[ainfo->unanchored_start_loop];
			if (!state_set_add(&s->epsilons, nfa->alloc, ainfo->eager_match_state)) {
				return false;
			}
			if (log) {
				fprintf(stderr, "%s: adding epsilon edge from unanchored_start_loop %d to eager_match_state %d\n",
				    __func__, ainfo->unanchored_start_loop, ainfo->eager_match_state);
			}
		}
	}

	/* If there are anchored ends, set an endid on them */
	if (!state_set_empty(ainfo->anchored_ends)) {
		struct state_iter si;
		state_set_reset(ainfo->anchored_ends, &si);
		fsm_state_t anchored_end_state;
		const fsm_end_id_t end_id = (fsm_end_id_t)(id + id_base);
		while (state_set_next(&si, &anchored_end_state)) {
			if (!fsm_endid_set(nfa, anchored_end_state, end_id)) {
				return false;
			}
			if (log) {
				fprintf(stderr, "%s: setting endid %d on anchored_end_state %d\n",
				    __func__, end_id, anchored_end_state);
			}
		}
	}

#if LOG_AFTER_MODIFY_GROUP_NFA
	fprintf(stderr, "=== after %s\n", __func__);
	fsm_print(stderr, nfa, &dump_nfa_opt, NULL, FSM_PRINT_DOT);
	fsm_endid_dump(stderr, nfa);
	fsm_eager_output_dump(stderr, nfa);

#endif

	return true;
}

static void
rebase_analysis_info(struct analysis_info *ainfo, fsm_state_t base)
{
	if (base == 0) { return; }

#define SHIFT(S) if (ainfo-> S != NO_STATE) { ainfo-> S += base; }
	SHIFT(start);
	SHIFT(unanchored_start_loop);
	SHIFT(unanchored_end_loop);
	SHIFT(unanchored_end_loop_end);
	SHIFT(eager_match_state);
#undef SHIFT

	state_set_rebase(&ainfo->anchored_ends, base);
	state_set_rebase(&ainfo->anchored_starts, base);
	state_set_rebase(&ainfo->eager_matches, base);
	state_set_rebase(&ainfo->reachable_from_nonstart_state, base);

	edge_set_rebase(&ainfo->anchored_firsts, base);
	edge_set_rebase(&ainfo->repeatable_firsts, base);

}

static void
free_analysis(const struct fsm_alloc *alloc, struct analysis_info *ainfo)
{
	state_set_free(ainfo->anchored_starts);
	state_set_free(ainfo->anchored_ends);
	state_set_free(ainfo->eager_matches);
	state_set_free(ainfo->needs_indirect_epsilon_edge_to_eager_match_state);
	state_set_free(ainfo->reachable_from_nonstart_state);
	edge_set_free(alloc, ainfo->anchored_firsts);
	edge_set_free(alloc, ainfo->repeatable_firsts);
}

/* Combine an array of FSMs into a single FSM that attempts to match them
 * all in one pass, with an extra loop so that more than one pattern with
 * eager outputs can match. */
struct fsm *
fsm_union_repeated_pattern_group(size_t nfa_count,
    struct fsm **nfas, struct fsm_combined_base_pair *bases, size_t id_base)
{
	const struct fsm_alloc *alloc = nfas[0]->alloc;
	const bool log = 0 || LOG_UNION_REPEATED_PATTERN_GROUP;

	struct analysis_info *ainfos = f_calloc(alloc, nfa_count, sizeof(ainfos[0]));
	if (ainfos == NULL) { goto fail; }

	size_t est_total_states = 0;
	for (size_t i = 0; i < nfa_count; i++) {
		assert(nfas[i]);
		if (nfas[i]->alloc != alloc) {
			errno = EINVAL;
			return NULL;
		}

		/* Any NFAs passed to this function must be built with
		 * an re_comp flag of RE_SAVE_LINKAGE_INFO, because some
		 * of the info saved during construction informs
		 * linking. */
		if (nfas[i]->linkage_info == NULL) {
			errno = EINVAL;
			return NULL;
		}

		const size_t count = fsm_countstates(nfas[i]);
		est_total_states += count;
	}

	for (size_t i = 0; i < nfa_count; i++) {
		struct fsm *fsm = nfas[i];

		struct analysis_info *ainfo = &ainfos[i];

		/* Copy these fields over, because fsm->linkage_info will be
		 * freed during the call to fsm_merge below. */
		{
			struct linkage_info *linkage_info = fsm->linkage_info;

			ainfo->unanchored_start_loop = linkage_info->unanchored_start_loop;
			ainfo->unanchored_end_loop = linkage_info->unanchored_end_loop;
			ainfo->unanchored_end_loop_end = linkage_info->unanchored_end_loop_end;

			/* Transfer ownership of these. */
			ainfo->anchored_starts = linkage_info->anchored_starts;
			linkage_info->anchored_starts = NULL;
			ainfo->anchored_ends = linkage_info->anchored_ends;
			linkage_info->anchored_ends = NULL;
		}

		/* Identify various states in the NFA that will be relevant to combining. */
		if (!analyze_group_nfa(fsm, ainfo)) {
			goto fail;
		}

		/* Change the NFA structure so it can better link into the combined FSM,
		 * and set endids and/or output IDs as appropriate. */
		if (!modify_group_nfa(fsm, i, &ainfos[i], id_base)) {
			goto fail;
		}
	}

	est_total_states += 5;	/* new start and end, new unanchored start and end loops */

	struct fsm *res = fsm_new_statealloc(alloc, est_total_states);
	if (res == NULL) { return NULL; }

	/* The new overall start state */
	fsm_state_t global_start;
	if (!fsm_addstate(res, &global_start)) { goto fail; }

	/* States linking to the starts of unanchored and anchored
	 * subgraphs, respectively. Matching group NFAs with unanchored
	 * ends will loop back to the global_unanchored_start_loop, but
	 * patterns anchored at the start are only reachable via
	 * global_anchored_start. */
	fsm_state_t global_unanchored_start_loop, global_anchored_start;
	if (!fsm_addstate(res, &global_unanchored_start_loop)) { goto fail; }
	if (!fsm_addstate(res, &global_anchored_start)) { goto fail; }

	/* The unanchored end loop state, and an end state with no outgoing edges. */
	fsm_state_t global_unanchored_end_loop, global_end;
	if (!fsm_addstate(res, &global_end)) { goto fail; }
	if (!fsm_addstate(res, &global_unanchored_end_loop)) { goto fail; }

	if (bases != NULL) {
		memset(bases, 0x00, nfa_count * sizeof(bases[0]));
	}

	/* For each group NFA, link its unanchored and anchored start states
	 * and eager_match_state to the global ones. */
	for (size_t nfa_i = 0; nfa_i < nfa_count; nfa_i++) {
		struct fsm *fsm = nfas[nfa_i];
		nfas[nfa_i] = NULL; /* transfer ownership */

		const size_t state_count = fsm_countstates(fsm);
		struct analysis_info *ainfo = &ainfos[nfa_i];
		if (ainfo->start == NO_STATE) {
			fsm_free(fsm);		      /* no start, just discard */
			continue;
		}
		assert(ainfo->start < state_count);

		/* Call fsm_merge; the argument order shouldn't matter. */
		struct fsm_combine_info combine_info;
		struct fsm *merged = fsm_merge(res, fsm, &combine_info);
		if (merged == NULL) { goto fail; }

		if (log) {
			fprintf(stderr, "merged: bases a %d and b %d\n", combine_info.base_a, combine_info.base_b);
		}

		/* Update offsets if res had its state IDs shifted forward. */
		global_start += combine_info.base_a;
		global_unanchored_start_loop += combine_info.base_a;
		global_anchored_start += combine_info.base_a;
		global_end += combine_info.base_a;
		global_unanchored_end_loop += combine_info.base_a;

		/* Also update offsets for the group FSM's states. */
		rebase_analysis_info(ainfo, combine_info.base_b);

		if (bases != NULL) {
			bases[nfa_i].state = combine_info.base_b;
			bases[nfa_i].capture = combine_info.capture_base_b;
		}

		/* Link the FSM's eager match state back to the global_unanchored_end_loop, so that after
		 * matching it in an unanchored way it can continue attempting to match other combined
		 * patterns that aren't anchored at their start. Also link it to the global end, so
		 * it will be retained during determinisation and minimisation. */
		if (ainfo->eager_match_state != NO_STATE) {
			if (!fsm_addedge_epsilon(merged, ainfo->eager_match_state, global_unanchored_end_loop)) {
				goto fail;
			}
			if (!fsm_addedge_epsilon(merged, ainfo->eager_match_state, global_end)) {
				goto fail;
			}
			if (log) {
				fprintf(stderr, "eager_match_state: adding epsilon EMS %d -> global_unanchored_end_loop %d and EMS %d -> global_end %d\n",
				    ainfo->eager_match_state, global_unanchored_end_loop,
				    ainfo->eager_match_state, global_end);
			}

			/* If the NFA matches the empty string and is not anchored at the end, then
			 * add an epsilon edge from the global start directly to its eager match state.
			 * This ensures all inputs will match this group NFA when combined. */
			if (ainfo->nullable) {
				assert(ainfo->unanchored_end_loop != NO_STATE);
				if (!fsm_addedge_epsilon(merged, global_start, ainfo->eager_match_state)) {
					goto fail;
				}
				if (log) {
					fprintf(stderr, "nullable: global_start %d -> eager_match_state %d\n",
					    global_start, ainfo->eager_match_state);
				}
			}
		} else {
			/* If the NFA matches an end-anchored empty string, then add an epsilon edge from
			 * the global start to an anchored end, which has an endid. */
			if (ainfo->nullable && !state_set_empty(ainfo->anchored_ends)) {
				struct state_iter si;
				state_set_reset(ainfo->anchored_ends, &si);
				fsm_state_t anchored_end_state;

				while (state_set_next(&si, &anchored_end_state)) {
					if (!fsm_addedge_epsilon(merged, global_start, anchored_end_state)) {
						goto fail;
					}
					if (log) {
						fprintf(stderr, "nullable & anchored ends: global_start %d -> anchored_end_state %d\n",
						    global_start, anchored_end_state);
					}

					/* It should only be necessary to link one, since that's enough
					 * for determinisation to carry the end id back to the start's
					 * epsilon closure. */
					break;
				}
			}
		}

		struct edge_group_iter iter;
		struct edge_group_iter_info info;

		/* Link the global_anchored_start to group FSM paths that are start-anchored
		 * and can only match at the start of input. */
		edge_set_group_iter_reset(ainfo->anchored_firsts, EDGE_GROUP_ITER_ALL, &iter);
		while (edge_set_group_iter_next(&iter, &info)) {
			assert(global_anchored_start < merged->statecount);
			struct fsm_state *anchored_start = &merged->states[global_anchored_start];
			if (!edge_set_add_bulk(&anchored_start->edges, merged->alloc,
				info.symbols, info.to)) {
				goto fail;
			}
			if (log) {
				fprintf(stderr, "anchored_firsts: adding global_anchored_start %d -> info.to %d (with same labels)\n",
				    global_anchored_start, info.to);
			}
		}

		/* Link the global_unanchored_start_loop to group FSM paths that aren't
		 * start-anchored. */
		edge_set_group_iter_reset(ainfo->repeatable_firsts, EDGE_GROUP_ITER_ALL, &iter);
		while (edge_set_group_iter_next(&iter, &info)) {
			struct fsm_state *unanchored_start = &merged->states[global_unanchored_start_loop];
			if (!edge_set_add_bulk(&unanchored_start->edges, merged->alloc,
				info.symbols, info.to)) {
				goto fail;
			}

			if (log) {
				fprintf(stderr, "repeatable_firsts: adding global_unanchored_start_loop %d -> info.to %d (same edges)\n",
				    global_unanchored_start_loop, info.to);
			}
		}


		/* Add an epsilon edge from the global unanchored start loop to the NFA's.
		 * Without this, eager outputs for eager matches in the start state's epsilon
		 * closure may get lost during determinisation. */
		if (ainfo->unanchored_start_loop != NO_STATE) {
		  	if (!fsm_addedge_epsilon(merged, global_unanchored_start_loop, ainfo->unanchored_start_loop)) {
		  		goto fail;
		  	}

			if (log) {
				fprintf(stderr, "repeatable_firsts: adding an epsilon edge from global_unanchored_start_loop %d to NFA unanchored_start_loop %d\n",
				    global_unanchored_start_loop, ainfo->unanchored_start_loop);
			}
		}

		res = merged;
	}

	/* Link the global start to the global unanchored start loop and anchored start states. */
	if (log) {
		fprintf(stderr, "linking: global_start %d -> global_unanchored_start_loop %d and global_anchored_start %d\n",
		    global_start, global_unanchored_start_loop, global_anchored_start);
	}
	if (!fsm_addedge_epsilon(res, global_start, global_unanchored_start_loop)) { goto fail; }
	if (!fsm_addedge_epsilon(res, global_start, global_anchored_start)) { goto fail; }

	/* Link the global unanchored start loop to itself, so it can
	 * consume and ignore input preceding each matching group NFA. */
	if (!fsm_addedge_any(res, global_unanchored_start_loop, global_unanchored_start_loop)) { goto fail; }

	/* Link the global unanchored end loop and global end. */
	if (log) {
		fprintf(stderr, "linking: global_unanchored_end_loop %d -> global_end %d (and -> self)\n", global_unanchored_end_loop, global_end);
	}
	if (!fsm_addedge_any(res, global_unanchored_end_loop, global_unanchored_end_loop)) { goto fail; }
	if (!fsm_addedge_epsilon(res, global_unanchored_end_loop, global_end)) { goto fail; }

	/* Link from the global_unanchored_end_loop to the global_unanchored_start_loop,
	 * so patterns with an unanchored start can follow other patterns with an unanchored
	 * end, possibly with other ignored input between them. */
	if (log || LOG_FSM_UNION_REPEATED_PATTERN_GROUP_OUTPUT) {
		fprintf(stderr, "%s: g_start %d, g_start_loop %d, g_start_anchored %d, g_end_loop %d, g_end %d (after all merging)\n",
		    __func__, global_start, global_unanchored_start_loop, global_anchored_start, global_unanchored_end_loop, global_end);
		fprintf(stderr, "%s: linking global_unanchored_end_loop %d to global_unanchored_start_loop %d\n",
		    __func__, global_unanchored_end_loop, global_unanchored_start_loop);
		fprintf(stderr, "%s: setting global_start %d and end %d\n", __func__, global_start, global_end);
	}
	if (!fsm_addedge_epsilon(res, global_unanchored_end_loop, global_unanchored_start_loop)) { goto fail; }

	/* Link the global unanchored end loop to the global end, so
	 * reaching the end of input there is considered a match. */
	if (!fsm_addedge_epsilon(res, global_unanchored_end_loop, global_end)) { goto fail; }

	/* These need to be set after merging, because that clears the start state. */
	fsm_setstart(res, global_start);
	fsm_setend(res, global_end, 1);

	for (size_t i = 0; i < nfa_count; i++) {
		free_analysis(alloc, &ainfos[i]);
	}

	if (LOG_UNION_REPEATED_PATTERN_GROUP || LOG_FSM_UNION_REPEATED_PATTERN_GROUP_OUTPUT) {
		fprintf(stderr, "==== %s output (combined, pre det+min)\n", __func__);
		fsm_print(stderr, res, &dump_nfa_opt, NULL, FSM_PRINT_DOT);
		fsm_endid_dump(stderr, res);
		fsm_eager_output_dump(stderr, res);
	}

	f_free(alloc, ainfos);

	return res;

fail:
	if (ainfos != NULL) {
		for (size_t i = 0; i < nfa_count; i++) {
			free_analysis(alloc, &ainfos[i]);
		}
		f_free(alloc, ainfos);
	}

	return NULL;
}
