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
#include <adt/set.h>
#include <adt/u64bitset.h>

#include "internal.h"

#define LOG_MAPPINGS 0
#define LOG_STEPS 0
#define LOG_TIME 0
#define LOG_INIT 0
#define LOG_ECS 0
#define LOG_PARTITIONS 0

#if LOG_TIME
#include <sys/time.h>
#endif

#include "minimise_internal.h"

int
fsm_minimise(struct fsm *fsm)
{
	int r = 0;
	struct fsm *dst = NULL;

	unsigned char labels[FSM_SIGMA_COUNT];
	size_t label_count, orig_states, minimised_states;
	fsm_state_t *mapping = NULL;
	unsigned *shortest_end_distance = NULL;

#if LOG_TIME
	struct timeval tv_pre, tv_post;

#define TIME(T) if (0 != gettimeofday(&T, NULL)) { assert(0); }
#define LOG_TIME_DELTA(NAME)				\
	fprintf(stderr, "%-8s %.3f msec\n", NAME,	\
	    1000.0 * (tv_post.tv_sec - tv_pre.tv_sec)	\
	    + (tv_post.tv_usec - tv_pre.tv_usec)/1000.0);
#else
#define TIME(T)
#define LOG_TIME_DELTA(NAME)
#endif

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

	TIME(tv_pre);
	collect_labels(fsm, labels, &label_count);
	TIME(tv_post);
	LOG_TIME_DELTA("collect_labels");

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

	TIME(tv_pre);
	r = build_minimised_mapping(fsm, labels, label_count,
	    shortest_end_distance,
	    mapping, &minimised_states);
	TIME(tv_post);
	LOG_TIME_DELTA("minimise");

	if (!r) {
		goto cleanup;
	}

	/* Minimisation should never add states. */
	assert(minimised_states <= orig_states);

	/* Use the mapping to consolidate the current states
	 * into a new DFA, combining states that could not be
	 * proven distinguishable. */
	TIME(tv_pre);
	dst = fsm_consolidate(fsm, mapping, fsm->statecount);
	TIME(tv_post);
	LOG_TIME_DELTA("consolidate");

	if (dst == NULL) {
		r = 0;
		goto cleanup;
	}

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

#if LOG_INIT
	for (i = 0; i < env.ec_count; i++) {
		fprintf(stderr, "# --ec[%lu]: %d\n", i, env.ecs[i]);
	}
#endif

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

				env.steps++;
				if (try_partition(&env, label,
					ec_src, ec_dst, pcounts)) {
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

#if EXPENSIVE_INTEGRITY_CHECKS
				check_done_ec_offset(&env);
#endif
			}
		}

		env.iter++;
	} while (changed);

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

#define PARTITION_BY_END_STATE_DISTANCE 1
#if PARTITION_BY_END_STATE_DISTANCE
/* Use counting sort to construct a permutation vector -- this is an
 * array of offsets into in[N] such that in[pv[0..N]] would give the
 * values of in[] in ascending order (but don't actually rearrange in,
 * just get the offsets). This is O(n). */
static unsigned *
build_permutation_vector(const struct fsm_alloc *alloc,
    size_t length, size_t max_value, unsigned *in)
{
	unsigned *out = NULL;
	unsigned *counts = NULL;
	size_t i;

	out = f_malloc(alloc, length * sizeof(*out));
	if (out == NULL) {
		goto cleanup;
	}
	counts = f_calloc(alloc, max_value + 1, sizeof(*out));
	if (counts == NULL) {
		goto cleanup;
	}

	/* Count each distinct value */
	for (i = 0; i < length; i++) {
		counts[in[i]]++;
	}

	/* Convert to cumulative counts, so counts[v] stores the upper
	 * bound for where sorting would place each distinct value. */
	for (i = 1; i <= max_value; i++) {
		counts[i] += counts[i - 1];
	}

	/* Sweep backwards through the input array, placing each value
	 * according to the cumulative count. Decrement the count so
	 * progressively earlier instances of the same value will
	 * receive earlier offsets in out[]. */
	for (i = 0; i < length; i++) {
	        const unsigned pos = length - i - 1;
		const unsigned value = in[pos];
		const unsigned count = --counts[value];
		out[count] = pos;
	}

	f_free(alloc, counts);
	return out;

cleanup:
	if (out != NULL) {
		f_free(alloc, out);
	}
	if (counts != NULL) {
		f_free(alloc, counts);
	}
	return NULL;
}
#endif

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
	pv = build_permutation_vector(fsm->opt->alloc,
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
	ranking = build_permutation_vector(fsm->opt->alloc,
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

#if EXPENSIVE_INTEGRITY_CHECKS
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
		if (i >= done_ec_offset) {
			assert(head == NO_ID || env->jump[head] == NO_ID);
		} else if (i >= 2) {
			assert(env->jump[head] != NO_ID);
		}
	}
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

static fsm_state_t
find_edge_destination(const struct fsm *fsm,
    fsm_state_t id, unsigned char label)
{
	struct fsm_edge e;

	assert(id < fsm->statecount);
	if (edge_set_find(fsm->states[id].edges, label, &e)) {
		return e.state;
	}

	return fsm->statecount;	/* dead state */
}

static int
try_partition(struct min_env *env, unsigned char label,
    fsm_state_t ec_src, fsm_state_t ec_dst,
	size_t partition_counts[2])
{
	fsm_state_t cur = MASK_EC_HEAD(env->ecs[ec_src]);
	fsm_state_t to, to_ec, first_ec, prev;

#if EXPENSIVE_INTEGRITY_CHECKS
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

	/* There must be at least two states in this EC.
	 * See where the current label leads on the first state.
	 * Any states which has an edge to a different EC for
	 * that label will be split into a new EC.
	 *
	 * Note that the ec_src EC is updated in place -- because this
	 * is successively trying different labels on the same EC, it
	 * can often do several partitions and make more progress in a
	 * single pass, avoiding most of the theoretical overhead of the
	 * fixpoint approach. */
	to = find_edge_destination(env->fsm, cur, label);
	first_ec = env->state_ecs[to];
	partition_counts[0] = 1;
	prev = cur;
	cur = env->jump[cur];

#if LOG_PARTITIONS > 1
	fprintf(stderr, "# --- try_partition: first, %u, has to %d -> first_ec %d\n", cur, to, first_ec);
#endif

	/* initialize the new EC -- empty */
	env->ecs[ec_dst] = NO_ID;

	while (cur != NO_ID) {
		to = find_edge_destination(env->fsm, cur, label);
		to_ec = env->state_ecs[to];
#if LOG_PARTITIONS > 1
		fprintf(stderr, "# --- try_partition: next, cur %u, has to %d -> to_ec %d\n", cur, to, to_ec);
#endif

		if (to_ec == first_ec) { /* in same EC */
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

#if EXPENSIVE_INTEGRITY_CHECKS
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
