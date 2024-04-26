/*
 * Copyright 2020 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <stdio.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>
#include <fsm/alloc.h>

#include <adt/edgeset.h>
#include <adt/pv.h>
#include <adt/set.h>
#include <adt/u64bitset.h>
#include <adt/hash.h>

#include "internal.h"
#include "capture.h"

#define LOG_MAPPINGS 0
#define LOG_STEPS 0
#define LOG_INIT 0
#define LOG_ECS 0
#define LOG_PARTITIONS 0

#include "minimise_internal.h"
#include "minimise_test_oracle.h"

#if EXPENSIVE_CHECKS
#include <fsm/print.h>

static void
check_done_ec_offset(const struct min_env *env);

static int
all_end_states_are_currently_together(const struct min_env *env);
#endif

static int
split_ecs_by_end_metadata(struct min_env *env, const struct fsm *fsm);

#define DEF_CAPTURE_ID_CEIL 4
struct end_metadata {
	struct end_metadata_end {
		unsigned count;
		fsm_end_id_t *ids;
	} end;
};

static int
collect_end_ids(const struct fsm *fsm, fsm_state_t s,
	struct end_metadata_end *e);

int
fsm_minimise(struct fsm *fsm)
{
	int r = 0;
	struct fsm *dst = NULL;

	unsigned char labels[FSM_SIGMA_COUNT];
	size_t label_count, orig_states, minimised_states;
	fsm_state_t *mapping = NULL;
	unsigned *shortest_end_distance = NULL;

	INIT_TIMERS();

	/* This should only be called with a DFA. */
	assert(fsm != NULL);
	assert(fsm_all(fsm, fsm_isdfa));

	/* The algorithm used below won't remove states without a path
	 * to an end state, because it cannot prove they're
	 * unnecessary, so they must be trimmed away first. */
	if (fsm_trim(fsm, FSM_TRIM_START_AND_END_REACHABLE,
		&shortest_end_distance) < 0) {
		return 0;
	}

	if (fsm->statecount == 0) {
		r = 1;
		goto cleanup;
	}

	TIME(&pre);
	collect_labels(fsm, labels, &label_count);
	TIME(&post);
	DIFF_MSEC("collect_labels", pre, post, NULL);

	if (label_count == 0) {
		r = 1;
		goto cleanup;
	}

	mapping = f_malloc(fsm->opt->alloc,
	    fsm->statecount * sizeof(mapping[0]));
	if (mapping == NULL) {
		goto cleanup;
	}

	orig_states = fsm->statecount;

	TIME(&pre);
	r = build_minimised_mapping(fsm, labels, label_count,
	    shortest_end_distance,
	    mapping, &minimised_states);
	TIME(&post);
	DIFF_MSEC("minimise", pre, post, NULL);

	if (!r) {
		goto cleanup;
	}

	/* Minimisation should never add states. */
	assert(minimised_states <= orig_states);

	/* Use the mapping to consolidate the current states
	 * into a new DFA, combining states that could not be
	 * proven distinguishable. */
	TIME(&pre);
	dst = fsm_consolidate(fsm, mapping, fsm->statecount);
	TIME(&post);
	DIFF_MSEC("consolidate", pre, post, NULL);

	if (dst == NULL) {
		r = 0;
		goto cleanup;
	}

#if EXPENSIVE_CHECKS
	if (!fsm_capture_has_captures(fsm)) {
		struct fsm *oracle = fsm_minimise_test_oracle(fsm);
		const size_t exp_count = fsm_countstates(oracle);
		const size_t got_count = fsm_countstates(dst);
		if (exp_count != got_count) {
			fprintf(stderr, "%s: expected minimal DFA with %zu states, got %zu\n",
			    __func__, exp_count, got_count);
			fprintf(stderr, "== expected:\n");
			fsm_print_fsm(stderr, oracle);

			fprintf(stderr, "== got:\n");
			fsm_print_fsm(stderr, dst);
			assert(!"non-minimal result");
		}

		fsm_free(oracle);
	}
#endif

	fsm_move(fsm, dst);

cleanup:
	if (mapping != NULL) {
		f_free(fsm->opt->alloc, mapping);
	}
	if (shortest_end_distance != NULL) {
		f_free(fsm->opt->alloc, shortest_end_distance);
	}

	return r;
}

/* Build a bit set of labels used, then write the set
 * into a sorted array. */
static void
collect_labels(const struct fsm *fsm,
    unsigned char *labels, size_t *label_count)
{
	uint64_t label_set[FSM_SIGMA_COUNT/64] = { 0, 0, 0, 0 };
	int i;

	fsm_state_t id;
	for (id = 0; id < fsm->statecount; id++) {
		struct edge_group_iter egi;
		edge_set_group_iter_reset(fsm->states[id].edges,
		    EDGE_GROUP_ITER_ALL, &egi);
		struct edge_group_iter_info info;
		while (edge_set_group_iter_next(&egi, &info)) {
			for (size_t i = 0; i < 4; i++) {
				label_set[i] |= info.symbols[i];
			}
		}
	}

	*label_count = 0;
	for (i = 0; i < 256; i++) {
		if (u64bitset_get(label_set, i)) {
			labels[*label_count] = i;
			(*label_count)++;
		} else if ((i & 63) == 0 && label_set[i/64] == 0) {
			i += 63; /* skip whole word */
		}
	}
}

/* Build a mapping for a minimised version of the DFA, using Moore's
 * algorithm (with a couple performance improvements).
 *
 * For a good description of the algorithm (albeit one that incorrectly
 * attributes it to Hopcroft) see pgs. 55-59 of Cooper & Torczon's
 * _Engineering a Compiler_. For another, see pg. 13-20 of Erin van der
 * Veen's thesis, _The Practical Performance of Automata Minimization
 * Algorithms_.
 *
 * Intuitively, the algorithm starts by dividing all of the states into
 * two sets, called Equivalence Classes (abbreviated EC below),
 * containing all non-final and final states respectively. Then, for
 * every EC, check whether any of its states' edges for the same label
 * lead to states in distinct ECs (and therefore observably different
 * results). If so, partition the EC into two ECs, containing the states
 * with matching and non-matching destination ECs. (Which EC is chosen
 * as matching doesn't affect correctness.) Repeat until no further
 * progress can be made, meaning the algorithm cannot prove anything
 * else is distinguishable via transitive final/non-final reachability.
 * Each EC represents a state in the minimised DFA mapping, and multiple
 * states in a single EC can be safely combined without affecting
 * observable behavior.
 *
 * When PARTITION_BY_END_STATE_DISTANCE is non-zero, instead of
 * starting with two ECs, do a pass grouping the states into ECs
 * according to their distance to the closest end state. See the
 * comments around it for further details. */
static int
build_minimised_mapping(const struct fsm *fsm,
    const unsigned char *dfa_labels, size_t dfa_label_count,
    const unsigned *shortest_end_distance,
    fsm_state_t *mapping, size_t *minimized_state_count)
{
	struct min_env env;
	int changed, res = 0;
	size_t i;
	fsm_state_t ec_offset;

	/* Alloc for each state, plus the dead state. */
	size_t alloc_size = (fsm->statecount + 1) * sizeof(env.state_ecs[0]);

	INIT_TIMERS();

	env.fsm = fsm;
	env.dead_state = fsm->statecount;
	env.iter = 0;
	env.steps = 0;

	assert(fsm->statecount > 0);
	env.state_ecs = NULL;
	env.jump = NULL;
	env.ecs = NULL;
	env.dfa_labels = dfa_labels;
	env.dfa_label_count = dfa_label_count;

	env.state_ecs = f_malloc(fsm->opt->alloc, alloc_size);
	if (env.state_ecs == NULL) { goto cleanup; }
	env.ec_map_count = fsm->statecount + 1;

	env.jump = f_malloc(fsm->opt->alloc, alloc_size);
	if (env.jump == NULL) { goto cleanup; }

	env.ecs = f_malloc(fsm->opt->alloc, alloc_size);
	if (env.ecs == NULL) { goto cleanup; }

	env.ecs[INIT_EC_NOT_FINAL] = NO_ID;
	env.ecs[INIT_EC_FINAL] = NO_ID;
	env.ec_count = 2;
	env.done_ec_offset = env.ec_count;

	if (!populate_initial_ecs(&env, fsm, shortest_end_distance)) {
		goto cleanup;
	}

	/* This only needs to be run once, but must run before the main
	 * fixpoint loop below, because it potentially refines ECs. */
	if (!split_ecs_by_end_metadata(&env, fsm)) {
		goto cleanup;
	}

#if LOG_INIT
	for (i = 0; i < env.ec_count; i++) {
		fprintf(stderr, "# --ec[%lu]: %d\n", i, env.ecs[i]);
	}
#endif

	TIME(&pre);
	do {		/* repeat until no further progress can be made */
		size_t l_i, ec_i;
		changed = 0;
		dump_ecs(stderr, &env);

		/* Check all active ECs for potential partitions. */
		for (ec_i = 0; ec_i < env.done_ec_offset; ec_i++) {
			int should_gather_EC_labels;
			struct min_label_iterator li;
	restart_EC:		/* ECs were swapped; restart the new EC here */
			{
				fsm_state_t next = env.ecs[ec_i];
				if (next == NO_ID) {
					continue;    /* 0 elements, skip */
				}
				should_gather_EC_labels = (next & SMALL_EC_FLAG) != 0;
				next = MASK_EC_HEAD(next);

				if (env.jump[next] == NO_ID) {
					continue;    /* only 1 element, skip */
				}
			}

			uint64_t checked_labels[256/64] = {0};
			init_label_iterator(&env, ec_i,
			    should_gather_EC_labels, &li);

			while (li.i < li.limit) {
				size_t pcounts[2];
				const fsm_state_t ec_src = ec_i;
				const fsm_state_t ec_dst = env.ec_count;
				unsigned char label;

				l_i = li.i;
				li.i++;

				label = (li.has_labels_for_EC
				    ? li.labels[l_i]
				    : env.dfa_labels[l_i]);

				/* This label has already been checked as part of
				 * another edge group, so we can safely skip it. */
				if (u64bitset_get(checked_labels, label)) {
					continue;
				}

				env.steps++;
				if (try_partition(&env, label,
					ec_src, ec_dst, pcounts, checked_labels)) {
					int should_restart_EC;
#if LOG_PARTITIONS > 0
					fprintf(stderr, "# partition: ec_i %lu/%u/%u, l_i %lu/%u%s, pcounts [ %lu, %lu ]\n",
					    ec_i, env.done_ec_offset, env.ec_count,
					    l_i, li.limit, li.has_labels_for_EC ? "*" : "",
					    pcounts[0], pcounts[1]);
#endif

					should_restart_EC =
					    update_after_partition(&env,
						pcounts, ec_src, ec_dst);

					changed = 1;
					env.ec_count++;
					assert(env.done_ec_offset <= env.ec_count);
					if (should_restart_EC) {
						goto restart_EC;
					}
				}

#if EXPENSIVE_CHECKS
				check_done_ec_offset(&env);
#endif
			}
		}

		env.iter++;
	} while (changed);
	TIME(&post);
	DIFF_MSEC("build_minimised_mapping__mainloop", pre, post, NULL);

	/* When all of the original input states are final, then the
	 * initial not-final EC will be empty. Skip it in the mapping,
	 * otherwise an unnecessary state will be created for it. */
	ec_offset = (env.ecs[INIT_EC_NOT_FINAL] == NO_ID) ? 1 : 0;

	/* Build map condensing to unique states */
	for (i = ec_offset; i < env.ec_count; i++) {
		fsm_state_t cur = MASK_EC_HEAD(env.ecs[i]);
		while (cur != NO_ID) {
			mapping[cur] = i - ec_offset;
			cur = env.jump[cur];
		}
	}

	if (minimized_state_count != NULL) {
		*minimized_state_count = env.ec_count - ec_offset;
	}

#if LOG_MAPPINGS
	for (i = 0; i < fsm->statecount; i++) {
		fprintf(stderr, "# minimised_mapping[%lu]: %u\n",
		    i, mapping[i]);
	}
#endif

#if LOG_STEPS
	fprintf(stderr, "# done in %lu iteration(s), %lu step(s), %ld -> %ld states, label_count %lu\n",
            env.iter, env.steps, fsm->statecount,
	    (size_t)(env.ec_count - ec_offset), env.dfa_label_count);
#endif

	res = 1;
	/* fall through */

cleanup:
	f_free(fsm->opt->alloc, env.ecs);
	f_free(fsm->opt->alloc, env.state_ecs);
	f_free(fsm->opt->alloc, env.jump);
	return res;
}

static void
dump_ecs(FILE *f, const struct min_env *env)
{
#if LOG_ECS
	size_t i = 0;
	fprintf(f, "# iter %ld, steps %lu\n", env->iter, env->steps);
	while (i < env->ec_count) {
		fsm_state_t cur = MASK_EC_HEAD(env->ecs[i]);
		fprintf(f, "M_EC[%lu]:", i);
		while (cur != NO_ID) {
			fprintf(f, " %u", cur);
			cur = env->jump[cur];
		}
		fprintf(f, "\n");
		i++;
	}
#else
	(void)f;
	(void)env;
#endif
}

#if EXPENSIVE_CHECKS
static void
check_descending_EC_counts(const struct min_env *env)
{
	size_t i, count, prev_count;
	fsm_state_t s = env->ecs[0];

	count = 0;
	while (s != NO_ID) {
		count++;
		s = env->jump[s];
	}
	prev_count = count;

	for (i = 1; i < env->ec_count; i++) {
		s = env->ecs[i];
		count = 0;
		while (s != NO_ID) {
			count++;
			s = env->jump[s];
		}
		assert(count <= prev_count);
		prev_count = count;
	}
}
#endif

#define PARTITION_BY_END_STATE_DISTANCE 1
static int
populate_initial_ecs(struct min_env *env, const struct fsm *fsm,
	const unsigned *shortest_end_distance)
{
	int res = 0;
	size_t i;

#if PARTITION_BY_END_STATE_DISTANCE
	/* Populate the initial ECs, partitioned by their shortest
	 * distance to an end state. Where Moore or Hopcroft's algorithm
	 * would typically start with two ECs, one for final states and
	 * one for non-final states, these states can be further
	 * partitioned into groups with equal shortest distances to an
	 * end state (0 for the end states themselves). This eliminates
	 * the worst case in `build_minimised_mapping`, where a very
	 * deeply nested path to an end state requires several passes:
	 * for example, an EC with 1000 states where only the last
	 * reaches the end state and is distinguishable, then 999, then
	 * 998, ... Using an initial partitioning based on the
	 * shortest_end_distance puts the states with each distance into
	 * their own ECs, replacing quadratic repetition. This initial
	 * pass can be done in linear time.
	 *
	 * This end-distance-based partitioning is described in
	 * _Efficient Deterministic Finite Automata Minimization Based
	 * on Backward Depth Information_ by Desheng Liu et. al. While
	 * I'm not convinced their approach works as presented -- and
	 * the pseudocode, diagrams, and test data tables in the paper
	 * contain numerous errors -- their proposition that any two
	 * states with different backwards depths to accept states must
	 * be distinguishable appears to be valid. We use this
	 * partitioning as a first pass, and then Moore's algorithm does
	 * the rest. */

	size_t count_ceil;
	unsigned *counts = NULL;
	unsigned *pv = NULL, *ranking = NULL;
	unsigned count_max = 0, sed_max = 0, sed_limit;

	assert(fsm != NULL);
	assert(shortest_end_distance != NULL);

	counts = f_calloc(fsm->opt->alloc,
	    DEF_INITIAL_COUNT_CEIL, sizeof(counts[0]));
	if (counts == NULL) {
		goto cleanup;
	}
	count_ceil = DEF_INITIAL_COUNT_CEIL;

	/* Count unique shortest_end_distances, growing the
	 * counts array as necessary, and track the max SED
	 * present. */
	for (i = 0; i < fsm->statecount; i++) {
		unsigned sed = shortest_end_distance[i];

#if LOG_INIT
		fprintf(stderr, "initial_ecs: %lu/%lu: sed %u\n",
		    i, fsm->statecount, sed);
#endif

		assert(sed != (unsigned)-1);
		if (sed >= count_ceil) {
			size_t ni;
			size_t nceil = 2 * count_ceil;
			unsigned *ncounts;
			while (sed >= nceil) {
				nceil *= 2;
			}
			ncounts = f_realloc(fsm->opt->alloc,
			    counts, nceil * sizeof(counts[0]));
			if (ncounts == NULL) {
				goto cleanup;
			}

			/* zero the newly allocated region */
			for (ni = count_ceil; ni < nceil; ni++) {
				ncounts[ni] = 0;
			}
			counts = ncounts;
			count_ceil = nceil;
		}

		counts[sed]++;
		if (sed > sed_max) {
			sed_max = sed;
		}
		if (counts[sed] > count_max) {
			count_max = counts[sed];
		}
	}

	/* The upper limit includes the max value. */
	sed_limit = sed_max + 1;

	/* Build a permutation vector of the counts, such
	 * that counts[pv[i..N]] would return the values
	 * in counts[] in ascending order. */
	pv = permutation_vector(fsm->opt->alloc,
	    sed_limit, count_max, counts);
	if (pv == NULL) {
		goto cleanup;
	}

	/* Build a permutation vector of the permutation vector,
	 * converting it into the rankings for counts[]. This is an
	 * old APL idiom[1], composing the grade-up operator (which
	 * builds an ascending permutation vector) with itself.
	 *
	 * Using k syntax & ngn's k [2] :
	 *
	 *   d:10?4		  / bind d to: draw 10 values 0 <= x < 4
	 *   d			  / print d's contents
	 * 3 3 0 1 3 1 1 3 2 2
	 *   <d			  / build permutation vector of d
	 * 2 3 5 6 8 9 0 1 4 7
	 *   d[<d]		  / d sliced by pv of d -> sorted d
	 * 0 1 1 1 2 2 3 3 3 3
	 *   d			  / print d's contents, again
	 * 3 3 0 1 3 1 1 3 2 2
	 *   <<d		  / pv of (pv of d): ranking vector of d
	 * 6 7 0 1 8 2 3 9 4 5
	 *			  / see how (0) -> 0, (1 2 3) -> 1,
	 *			  / (4 5) -> 2, (6 7 8 9) -> 3?
	 *   9-<<d		  / subtract from the length to count
	 * 3 2 9 8 1 7 6 0 5 4	  / from the end, for descending ranks.
	 *			  / now (0 1 2 3) -> 3, (4 5) -> 2, ...
	 *   dr:{(-1+#x)-<<x}	  / bind function: descending rank
	 *   dr d		  / put it all together
	 * 3 2 9 8 1 7 6 0 5 4
	 *			  / one more example, to show the pattern
	 *   d:5 5 5 2 2 2 2 1 1 1
	 *   dr d
	 * 2 1 0 6 5 4 3 9 8 7	  / (2 1 0) -> 5, (6 5 4 3) -> 2, (9 8 7) -> 1
	 *
	 * [1]: http://www.sudleyplace.com/APL/Anatomy%20of%20An%20Idiom.pdf
	 * [2]: https://bitbucket.org/ngn/k/src
	 */
	ranking = permutation_vector(fsm->opt->alloc,
	    sed_limit, sed_limit, pv);
	if (ranking == NULL) {
		goto cleanup;
	}

	/* Reverse the ranking offsets -- count from the end, rather
	 * than the start, so ECs with higher counts appear first. */
	for (i = 0; i < sed_limit; i++) {
		ranking[i] = sed_max - ranking[i];
		env->ecs[i] = NO_ID;
	}

	/* Assign the states to the ECs, ordered by descending ranking.
	 * We want the largest ECs first, as they will need the most
	 * processing. All ECs with less than 2 states are already done,
	 * so they should be together at the end. */
	for (i = 0; i < fsm->statecount; i++) {
		const unsigned sed = shortest_end_distance[i];
		fsm_state_t ec;
		assert(sed < sed_limit);

		/* assign EC and link state at head of list */
		ec = ranking[sed];
		env->state_ecs[i] = ec;
		env->jump[i] = env->ecs[ec];
		env->ecs[ec] = i;
	}

	/* Set done_ec_offset to the first EC with <2 states, if any. */
	env->ec_count = sed_limit;
	env->done_ec_offset = env->ec_count;
	for (i = 0; i < sed_limit; i++) {
		const unsigned count = counts[shortest_end_distance[env->ecs[i]]];
		if (count < 2) {
			env->done_ec_offset = i;
			break;
		}
	}

	/* The dead state is not a member of any EC. */
	env->state_ecs[env->dead_state] = NO_ID;

#if EXPENSIVE_CHECKS
	check_descending_EC_counts(env);
#endif

	res = 1;

cleanup:
	f_free(fsm->opt->alloc, counts);
	f_free(fsm->opt->alloc, pv);
	f_free(fsm->opt->alloc, ranking);
	return res;

#else
	(void)shortest_end_distance;

	for (i = 0; i < fsm->statecount; i++) {
		const fsm_state_t ec = fsm_isend(fsm, i)
		    ? INIT_EC_FINAL : INIT_EC_NOT_FINAL;
		env->state_ecs[i] = ec;
		/* link at head of the list */
		env->jump[i] = env->ecs[ec];
		env->ecs[ec] = i;
#if LOG_INIT
		fprintf(stderr, "# --init[%lu]: ec %d, jump[]= %d\n",
		    i, ec, env->jump[i]);
#endif
	}

	/* The dead state is not a member of any EC. */
	env->state_ecs[env->dead_state] = NO_ID;
	res = 1;
	return res;
#endif
}

SUPPRESS_EXPECTED_UNSIGNED_INTEGER_OVERFLOW()
static void
incremental_hash_of_ids(uint64_t *accum, fsm_end_id_t id)
{
	(*accum) += hash_id(id);
}

static int
same_end_metadata(const struct end_metadata *a, const struct end_metadata *b)
{
	if (a->end.count != b->end.count) {
		return 0;
	}

	/* compare -- these must be sorted */

	for (size_t i = 0; i < a->end.count; i++) {
		if (a->end.ids[i] != b->end.ids[i]) {
			return 0;
		}
	}

	return 1;
}

static int
split_ecs_by_end_metadata(struct min_env *env, const struct fsm *fsm)
{
	int res = 0;

	struct end_metadata *end_md;
	fsm_state_t *htab = NULL;

	const size_t state_count = fsm_countstates(fsm);

#if EXPENSIVE_CHECKS
	/* Invariant: For each EC, either all or none of the states
	 * are end states. We only partition the set(s) of end states
	 * here. */
	all_end_states_are_currently_together(env);
#endif

	/* Use the hash table to assign to new groups. */

	end_md = f_calloc(fsm->opt->alloc,
	    state_count, sizeof(end_md[0]));
	if (end_md == NULL) {
		goto cleanup;
	}

	size_t bucket_count = 1;
	while (bucket_count < state_count) {
		bucket_count *= 2; /* power of 2 ceiling */
	}
	const size_t mask = bucket_count - 1;

	htab = f_malloc(fsm->opt->alloc,
	    bucket_count * sizeof(htab[0]));
	if (htab == NULL) {
		goto cleanup;
	}

	/* First pass: collect end state metadata */
	for (size_t ec_i = 0; ec_i < env->ec_count; ec_i++) {
		fsm_state_t s = MASK_EC_HEAD(env->ecs[ec_i]);
#if LOG_ECS
		fprintf(stderr, "## EC %zu\n", ec_i);
#endif
		while (s != NO_ID) {
			struct end_metadata *e = &end_md[s];
			if (!fsm_isend(fsm, s)) {
				break; /* this EC has non-end states, skip */
			}

			if (!collect_end_ids(fsm, s, &e->end)) {
				goto cleanup;
			}

			s = env->jump[s];
		}
	}

#if LOG_ECS
	fprintf(stderr, "==== BEFORE PARTITIONING BY END METADATA\n");
	dump_ecs(stderr, env);
	fprintf(stderr, "====\n");
#endif

	/* Second pass: partition ECs into groups with identical end IDs.
	 * for each group with different end IDs, unlink them. */
	const size_t max_ec = env->ec_count;
	for (size_t ec_i = 0; ec_i < max_ec; ec_i++) {
		fsm_state_t s = MASK_EC_HEAD(env->ecs[ec_i]);
		fsm_state_t prev = NO_ID;

		for (size_t i = 0; i < bucket_count; i++) {
			htab[i] = NO_ID; /* reset hash table */
		}

		while (s != NO_ID) {
			const struct end_metadata *s_md = &end_md[s];

			uint64_t hash = 0;
			const fsm_state_t next = env->jump[s];

			for (size_t eid_i = 0; eid_i < s_md->end.count; eid_i++) {
				incremental_hash_of_ids(&hash, s_md->end.ids[eid_i]);
			}

			for (size_t b_i = 0; b_i < bucket_count; b_i++) {
				fsm_state_t *b = &htab[(b_i + hash) & mask];
				const fsm_state_t other = *b;
				const struct end_metadata *other_md = &end_md[other];

				if (other == NO_ID) { /* empty hash bucket */
					*b = s;
					if (prev == NO_ID) {
						/* keep the first state, along with other states
						 * with matching end IDs, in this EC. no-op. */
#if LOG_ECS
						fprintf(stderr, " -- keeping state s %d in EC %u\n",
						    s, env->state_ecs[s]);
#endif
						prev = s;
					} else { /* not first (prev is set), so it landed somewhere else */
						/* unlink and assign new EC */
#if LOG_ECS
						fprintf(stderr, " -- moving state s %d from EC %u to EC %u\n",
						    s, env->state_ecs[s], env->ec_count);
#endif
						env->jump[prev] = env->jump[s]; /* unlink */
						env->ecs[env->ec_count] = s;    /* head of new EC */
						env->state_ecs[s] = env->ec_count;
						env->jump[s] = NO_ID;
						env->ec_count++;
					}
					break;
				} else if (same_end_metadata(s_md, other_md)) {
					if (env->state_ecs[other] == ec_i) {
						/* keep in the current EC -- no-op */
#if LOG_ECS
						fprintf(stderr, " -- keeping state s %d in EC %u\n",
						    s, env->state_ecs[s]);
#endif
						prev = s;
					} else {
						/* unlink and link to other state's EC */
#if LOG_ECS
						fprintf(stderr, " -- appending s %d to EC %u, after state %d, before %d\n",
						    s, env->state_ecs[other], other, env->jump[other]);
#endif
						assert(prev != NO_ID);
						env->jump[prev] = env->jump[s]; /* unlink */
						env->state_ecs[s] = env->state_ecs[other];
						env->jump[s] = env->jump[other];
						env->jump[other] = s; /* link after other */
					}
					break;
				} else {
					continue; /* collision */
				}
			}

			s = next;
		}

		/* If this EC only has one entry and it's before the
		 * done_ec_offset, then set that here so that invariants
		 * will be restored while sweeping forward after this loop. */

		if (env->jump[MASK_EC_HEAD(env->ecs[ec_i])] == NO_ID && ec_i < env->done_ec_offset) {
			env->done_ec_offset = ec_i; /* will be readjusted later */
		}

#if LOG_ECS
		fprintf(stderr, "==== AFTER PARTITIONING BY END METADATA -- EC %zu\n", ec_i);
		dump_ecs(stderr, env);
		fprintf(stderr, "==== (done_ec_offset: %d)\n", env->done_ec_offset);
#endif
	}

#if LOG_ECS
	fprintf(stderr, "==== AFTER PARTITIONING BY END IDs\n");
	dump_ecs(stderr, env);
	fprintf(stderr, "==== (done_ec_offset: %d)\n", env->done_ec_offset);
#endif

	/* Sweep forward and swap ECs as necessary so all single-entry
	 * ECs are at the end -- they're done. */
	size_t ec_i = env->done_ec_offset;

	while (ec_i < env->ec_count) {
		const fsm_state_t head = MASK_EC_HEAD(env->ecs[ec_i]);
		if (env->jump[head] == NO_ID) {
			/* offset stays where it is */
#if LOG_ECS
			fprintf(stderr, "ec_i: %zu / %u -- branch a\n", ec_i, env->ec_count);
#endif
			env->ecs[ec_i] = SET_SMALL_EC_FLAG(head);
		} else {
			/* this EC has more than one state, but is after
			 * the done_ec_offset, so swap it with an EC at
			 * the boundary. */
			const fsm_state_t n_ec_i = env->done_ec_offset;
#if LOG_ECS
			fprintf(stderr, "ec_i: %zu / %u -- branch b -- swap %ld and %d\n",
			    ec_i, env->ec_count, ec_i, n_ec_i);
#endif

			/* swap ec[n_ec_i] and ec[ec_i] */
			const fsm_state_t tmp = env->ecs[ec_i];
			env->ecs[ec_i] = env->ecs[n_ec_i];
			env->ecs[n_ec_i] = tmp;
			/* note: this may set the SMALL_EC_FLAG. */
			update_ec_links(env, ec_i);
			update_ec_links(env, n_ec_i);
			env->done_ec_offset++;
		}
		ec_i++;
	}

#if LOG_ECS
	fprintf(stderr, "==== (done_ec_offset is now: %d, ec_count %u)\n", env->done_ec_offset, env->ec_count);
	dump_ecs(stderr, env);
#endif

	/* check that all ECs are before/after done_ec_offset */
	for (size_t ec_i = 0; ec_i < env->ec_count; ec_i++) {
		const fsm_state_t s = MASK_EC_HEAD(env->ecs[ec_i]);
#if LOG_ECS
		fprintf(stderr, "  -- ec_i %zu: s %d\n", ec_i, s);
#endif
		if (ec_i < env->done_ec_offset) {
			assert(env->jump[s] != NO_ID);
		} else {
			assert(env->jump[s] == NO_ID);
		}
	}

	res = 1;

cleanup:
	if (htab != NULL) {
		f_free(fsm->opt->alloc, htab);
	}
	if (end_md != NULL) {
		size_t i;
		for (i = 0; i < state_count; i++) {
			struct end_metadata *e = &end_md[i];
			if (e->end.ids != NULL) {
				f_free(fsm->opt->alloc, e->end.ids);
			}
		}
		f_free(fsm->opt->alloc, end_md);
	}

	return res;
}

static int
cmp_end_ids(const void *pa, const void *pb)
{
	const fsm_end_id_t a = *(fsm_end_id_t *)pa;
	const fsm_end_id_t b = *(fsm_end_id_t *)pb;
	return a < b ? -1 : a > b ? 1 : 0;
}

static int
collect_end_ids(const struct fsm *fsm, fsm_state_t s,
	struct end_metadata_end *e)
{
	e->count = fsm_getendidcount(fsm, s);

	if (e->count > 0) {
		e->ids = f_malloc(fsm->opt->alloc,
		    e->count * sizeof(e->ids[0]));
		if (e->ids == NULL) {
			return 0;
		}

		size_t written;
		enum fsm_getendids_res res = fsm_getendids(fsm, s,
		    e->count, e->ids, &written);
		assert(res == FSM_GETENDIDS_FOUND);
		assert(written == e->count);

		/* sort, to make comparison easier later */
		qsort(e->ids, e->count,
		    sizeof(e->ids[0]), cmp_end_ids);

#if LOG_ECS
		fprintf(stderr, "%d:", s);
		for (size_t i = 0; i < written; i++) {
			fprintf(stderr, " %u", e->ids[i]);
		}
		fprintf(stderr, "\n");
#endif
	}
	return 1;
}

#if EXPENSIVE_CHECKS
static void
check_done_ec_offset(const struct min_env *env)
{
	size_t i;
	/* All ECs after the done_ec_offset have less than two elements
	 * (and cannot be partitioned further), all elements after the
	 * first two but before the offset have two or more. The first
	 * two are allowed to to have less, because for example the DFA
	 * may start with only one end state. The done_ec_offset is used
	 * to avoid redundantly scanning lots of small ECs that have
	 * been split off from the initial sets, but it would not be
	 * worth the added complexity to avoid checking ECs 0 and 1. */
	for (i = 0; i < env->ec_count; i++) {
		const fsm_state_t head = MASK_EC_HEAD(env->ecs[i]);
		if (i >= env->done_ec_offset) {
			assert(head == NO_ID || env->jump[head] == NO_ID);
		} else if (i >= 2) {
			assert(env->jump[head] != NO_ID);
		}
	}
}

static int
all_end_states_are_currently_together(const struct min_env *env)
{
	/* For each EC, either all or none of the states in it
	 * are end states. */
	for (size_t i = 0; i < env->ec_count; i++) {
		const fsm_state_t head = MASK_EC_HEAD(env->ecs[i]);
		const int ec_first_is_end = fsm_isend(env->fsm, head);

		fsm_state_t s = env->jump[head];
		while (s != NO_ID) {
			if (fsm_isend(env->fsm, s) != ec_first_is_end) {
				return 0;
			}
			s = env->jump[s];
		}
	}

	return 1;
}
#endif

static int
update_after_partition(struct min_env *env,
    const size_t *partition_counts,
    fsm_state_t ec_src, fsm_state_t ec_dst)
{
	int should_restart_EC = 0;
	assert(partition_counts[0] > 0);
	assert(partition_counts[1] > 0);

	/* ec_dst is at the end, so ecs[ec_dst] is in the
	 * done group by default. If there's only one,
	 * leave it there. */
	if (partition_counts[1] == 1) {
		assert(env->done_ec_offset > 0);
		update_ec_links(env, ec_dst);
	} else {
		/* Otherwise, swap and increment the offset so that dst
		 * is moved immediately before the done group, but
		 * outside of it. */
		const fsm_state_t ndst = env->done_ec_offset;
		const fsm_state_t tmp = env->ecs[ec_dst];
		env->ecs[ec_dst] = env->ecs[ndst];
		env->ecs[ndst] = tmp;

		assert(partition_counts[1] > 1); /* not 0 */

		update_ec_links(env, ec_dst);
		update_ec_links(env, ndst);
		env->done_ec_offset++;
	}

	/* If the src EC only has one, swap it immediately before
	 * the done group and decrement the offset, expanding it
	 * to include the src EC. */
	if (partition_counts[0] == 1 && env->done_ec_offset > 0) {
		fsm_state_t nsrc = env->done_ec_offset - 1;

		/* swap ec[nsrc] and ec[ec_src] */
		const fsm_state_t tmp = env->ecs[ec_src];
		env->ecs[ec_src] = env->ecs[nsrc];
		env->ecs[nsrc] = tmp;
		update_ec_links(env, ec_src);
		update_ec_links(env, nsrc);

		assert(env->done_ec_offset > 0);
		env->done_ec_offset--;

		/* The src EC just got swapped with another,
		 * so start over at the beginning of the labels
		 * for the new EC. */
		should_restart_EC = 1;
	}

	return should_restart_EC;
}

static void
update_ec_links(struct min_env *env, fsm_state_t ec_dst)
{
	size_t count = 0;
	fsm_state_t cur = env->ecs[ec_dst];

	assert(cur != NO_ID);
	cur = MASK_EC_HEAD(cur);

	while (cur != NO_ID) {
		env->state_ecs[cur] = ec_dst;
		cur = env->jump[cur];
		count++;
	}

	/* If the EC count is smaller than the threshold (and it isn't a
	 * DFA with a very small number of labels overall), then set the
	 * flag indicating that it should get smarter label checking. */
	if (count <= SMALL_EC_THRESHOLD
		&& env->dfa_label_count >= DFA_LABELS_THRESHOLD) {
		env->ecs[ec_dst] = SET_SMALL_EC_FLAG(env->ecs[ec_dst]);
	}
}

static void
init_label_iterator(const struct min_env *env,
	fsm_state_t ec_i, int should_gather_EC_labels,
	struct min_label_iterator *li)
{
	/* If the SMALL_EC flag was set for the EC (indicating that it has a
	 * small number of states), then gather the labels actually used
	 * by those states rather than checking all labels in the entire
	 * DFA. */
	if (should_gather_EC_labels) {
		fsm_state_t cur;
		size_t i;
		uint64_t label_set[FSM_SIGMA_COUNT/64 + 1] = { 0 };

		cur = env->ecs[ec_i];
		assert(cur != NO_ID);
		cur = MASK_EC_HEAD(cur);

		while (cur != NO_ID) {
			struct edge_group_iter egi;
			edge_set_group_iter_reset(env->fsm->states[cur].edges,
			    EDGE_GROUP_ITER_ALL, &egi);
			struct edge_group_iter_info info;
			while (edge_set_group_iter_next(&egi, &info)) {
				for (size_t i = 0; i < 4; i++) {
					label_set[i] |= info.symbols[i];
				}
			}
			cur = env->jump[cur];
		}

		li->has_labels_for_EC = 1;
		li->i = 0;
		li->limit = 0;
		i = 0;
		while (i < 256) {
			/* Check the bitset, skip forward 64 entries at a time
			 * if they're all zero. */
			if ((i & 63) == 0 && label_set[i/64] == 0) {
				i += 64;
			} else {
				if (u64bitset_get(label_set, i)) {
					li->labels[li->limit] = i;
					li->limit++;
				}
				i++;
			}
		}
	} else {		/* check all labels in sequence */
		li->has_labels_for_EC = 0;
		li->limit = env->dfa_label_count;
		li->i = 0;
	}
}

static int
try_partition(struct min_env *env, unsigned char label,
    fsm_state_t ec_src, fsm_state_t ec_dst,
    size_t partition_counts[2], uint64_t checked_labels[256/64])
{
	fsm_state_t cur = MASK_EC_HEAD(env->ecs[ec_src]);
	fsm_state_t prev, first_dst_state;
	unsigned to_ec, first_ec;

	const unsigned dead_state_ec = env->state_ecs[env->dead_state];
	const struct fsm_state *states = env->fsm->states;

#if EXPENSIVE_CHECKS
	/* Count states here, to compare against the partitioned
	 * EC' counts later. */
	size_t state_count = 0, psrc_count, pdst_count;
	while (cur != NO_ID) {
		cur = env->jump[cur];
		state_count++;
	}
	cur = MASK_EC_HEAD(env->ecs[ec_src]);
#endif

	memset(partition_counts, 0x00, 2*sizeof(partition_counts[0]));

#if LOG_PARTITIONS > 1
	fprintf(stderr, "# --- try_partition: checking '%c' for %u\n", label, ec_src);
#endif

	uint64_t first_dst_label_set[256/64];

	/* Use the edge_set's label grouping to check a
	 * set of labels at once, and note which labels
	 * have already been checked. */
	if (edge_set_check_edges_with_EC_mapping(states[cur].edges, label,
		env->ec_map_count, env->state_ecs,
		&first_dst_state, first_dst_label_set)) {

		assert(first_dst_state < env->ec_map_count);
		first_ec = env->state_ecs[first_dst_state];

		/* Note that all of these labels are being checked at once
		 * in this step, so they can be skipped in the caller's loop. */
		for (size_t w_i = 0; w_i < 256/64; w_i++) {
			checked_labels[w_i] |= first_dst_label_set[w_i];
		}
	} else {
		/* not found: set to the EC for the dead state */
		first_dst_state = env->dead_state;
		first_ec = dead_state_ec;
	}
#if LOG_PARTITIONS > 1
		fprintf(stderr, "# --- try_partition: label '%c' -> EC %d\n", label, first_ec);
#endif

	partition_counts[0] = 1;
	prev = cur;
	cur = env->jump[cur];

#if LOG_PARTITIONS > 1
	fprintf(stderr, "# --- try_partition: first, %u, has first_dst_state %d -> first_ec %d\n", cur, first_dst_state, first_ec);
#endif

	/* initialize the new EC -- empty */
	env->ecs[ec_dst] = NO_ID;

	while (cur != NO_ID) {
		uint64_t cur_label_set[256/64];
		fsm_state_t cur_dst_state;

		if (edge_set_check_edges_with_EC_mapping(states[cur].edges, label,
			env->ec_map_count, env->state_ecs,
			&cur_dst_state, cur_label_set)) {
			assert(cur_dst_state < env->ec_map_count);
			to_ec = env->state_ecs[cur_dst_state];
		} else {
			/* not found: set to the EC for the dead state */
			cur_dst_state = env->dead_state;
			to_ec = dead_state_ec;
		}

#if LOG_PARTITIONS > 1
		fprintf(stderr, "# --- try_partition: next, cur %u -> to_ec %d\n", cur, to_ec);
#endif

		/* If they're in the same EC and the label sets match, keep it in the
		 * EC, otherwise unlink and split it into a new EC. */
		if (to_ec == first_ec
		    && label_sets_match(first_dst_label_set, cur_label_set)) {
			partition_counts[0]++;
			prev = cur;
			cur = env->jump[cur];
		} else {	/* unlink, split */
			fsm_state_t next;
#if LOG_PARTITIONS > 1
			fprintf(stderr, "# try_partition: unlinking -- label '%c', src %u, dst %u, first_ec %d, cur %u -> to_ec %d\n", label, ec_src, ec_dst, first_ec, cur, to_ec);
#endif

			/* Unlink this state from the current EC,
			 * instead put it as the start of a new one
			 * at ecs[ec_dst]. */
			next = env->jump[cur];
			env->jump[prev] = next;
			env->jump[cur] = env->ecs[ec_dst];
			env->ecs[ec_dst] = cur;
			cur = next;
			partition_counts[1]++;
		}
	}

#if EXPENSIVE_CHECKS
	/* Count how many states were split into each EC
	 * and check that the sum matches the original count. */
	psrc_count = 0;
	cur = env->ecs[ec_src];
	if (cur != NO_ID) {
		cur = MASK_EC_HEAD(cur);
		while (cur != NO_ID) {
			cur = env->jump[cur];
			psrc_count++;
		}
	}

	pdst_count = 0;
	cur = env->ecs[ec_dst];
	if (cur != NO_ID) {
		cur = MASK_EC_HEAD(cur);
		while (cur != NO_ID) {
			cur = env->jump[cur];
			pdst_count++;
		}
	}

	assert(state_count == psrc_count + pdst_count);
#endif

	return partition_counts[1] > 0;
}

static int
label_sets_match(const uint64_t a[256/64], const uint64_t b[256/64])
{
	for (size_t i = 0; i < 256/64; i++) {
		if (a[i] != b[i]) {
			return 0;
		}
	}
	return 1;
}
