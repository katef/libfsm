/*
 * Copyright 2008-2017 Katherine Flavel
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
#include <adt/queue.h>
#include <adt/set.h>

#include "internal.h"

#define MIN_REAL 1
#define MIN_ALT 0
#define DUMP_MAPPINGS 0
#define DUMP_STEPS 0
#define DUMP_TIME 0

#define NAIVE_DUMP_TABLE 0
#define NAIVE_LOG_PARTITION 0

#define MOORE_DUMP_INIT 0
#define MOORE_DUMP_ECS 0
#define MOORE_LOG_PARTITION 0

#define EXMOORE_DUMP_INIT 1
#define EXMOORE_DUMP_ECS 1
#define EXMOORE_LOG_PARTITION 1

#define DO_HOPCROFT 0
#define HOPCROFT_DUMP_INIT 0
#define HOPCROFT_DUMP_ECS 0
#define HOPCROFT_LOG_STACK 0
#define HOPCROFT_LOG_PARTITION 0
#define HOPCROFT_LOG_FREELIST 0
#define HOPCROFT_LOG_INDEX 0
#define HOPCROFT_USE_LABEL_INDEX 0

#define EXPENSIVE_INTEGRITY_CHECKS 0

#if DUMP_TIME
#include <sys/time.h>
#endif

#if MIN_ALT
#include "minimise_internal.h"
#else
static int
min_brz(struct fsm *fsm, size_t *minimized_state_count);
#endif




#if MIN_REAL
#define NO_ID ((fsm_state_t)-1)

struct exmoore_env {
	const struct fsm *fsm;
	fsm_state_t dead_state;
	fsm_state_t *state_ecs;
	fsm_state_t *jump;
	fsm_state_t *ecs;
	const unsigned char *labels;
	size_t label_count;
};

struct exmoore_label_iterator {
	unsigned i;
	unsigned limit;
	unsigned char use_special;
	unsigned char labels[256];
};

static int
min_exmoore(const struct fsm *fsm,
    const unsigned char *labels, size_t label_count,
    fsm_state_t *mapping, size_t *minimized_state_count);

static void
exmoore_dump_ec(FILE *f, fsm_state_t start, const fsm_state_t *jump);

static int
exmoore_try_partition(struct exmoore_env *env, unsigned char label,
    fsm_state_t ec_src, fsm_state_t ec_dst,
    size_t partition_counts[2]);

static void
exmoore_heuristic_scan(FILE *f, const struct exmoore_env *env,
    const unsigned char *labels, size_t label_count);

static void
exmoore_init_label_iterator(const struct exmoore_env *env,
	fsm_state_t ec_i, int special_handling,
	struct exmoore_label_iterator *li);
#endif










#if MIN_REAL || MIN_ALT
/* Build a bit set of labels used, then write the set
 * into a sorted array. */
static void
collect_labels(const struct fsm *fsm,
    unsigned char *labels, size_t *label_count)
{
	size_t count = 0;
	uint64_t label_set[4] = { 0, 0, 0, 0 };
	int i;

	fsm_state_t id;
	for (id = 0; id < fsm->statecount; id++) {
		struct fsm_edge e;
		struct edge_iter ei;
		unsigned char label;
		for (edge_set_reset(fsm->states[id].edges, &ei);
		     edge_set_next(&ei, &e); ) {
			if (e.state >= fsm->statecount) {
				fprintf(stderr, "### ERROR: e.state %u >= fsm->statecount %lu\n",
				    e.state, fsm->statecount);
				continue;
			}
			assert(e.state < fsm->statecount);
			label = e.symbol;

			if (label_set[label/64] & (1UL << (label & 63))) {
				/* already set, ignore */
			} else {
				label_set[label/64] |= (1UL << (label & 63));
				count++;
			}
		}
	}

	*label_count = 0;
	for (i = 0; i < 256; i++) {
		if (label_set[i/64] & (1UL << (i & 63))) {
			labels[*label_count] = i;
			(*label_count)++;
		}
	}

	assert(*label_count == count);
}
#endif

int
fsm_minimise(struct fsm *fsm)
{
#if MIN_REAL
	/* build label mapping */
	unsigned char labels[256];
	size_t label_count, minimised_states;
	struct fsm *dst = NULL;
	fsm_state_t *mapping = NULL;
	int r = 0;

#if DUMP_TIME
	struct timeval tv_pre, tv_post;

#define TIME(T) if (0 != gettimeofday(&T, NULL)) { assert(0); }
#define DUMP_TIME_DELTA(NAME)				\
	fprintf(stderr, "%-8s %.3f msec\n", NAME,	\
	    1000.0 * (tv_post.tv_sec - tv_pre.tv_sec)	\
	    + (tv_post.tv_usec - tv_pre.tv_usec)/1000.0);
#else
#define TIME(T)
#define DUMP_TIME_DELTA(NAME)
#endif

	assert(fsm != NULL);

	/* This should only be called with a DFA. */
	assert(fsm_all(fsm, fsm_isdfa));

	if (fsm_trim(fsm, FSM_TRIM_START_AND_END_REACHABLE) < 0) {
		return 0;
	}

	if (fsm->statecount == 0) {
		return 1;	/* empty -- no-op */
	}

	TIME(tv_pre);
	collect_labels(fsm, labels, &label_count);
	TIME(tv_post);
	DUMP_TIME_DELTA("collect_labels");

	if (label_count == 0) {
		return 1;	/* empty -- no-op */
	}

	mapping = f_malloc(fsm->opt->alloc,
	    fsm->statecount * sizeof(mapping[0]));
	if (mapping == NULL) {
		goto cleanup;
	}

	/* exmoore */
	TIME(tv_pre);
	r = min_exmoore(fsm, labels, label_count, mapping, &minimised_states);
	TIME(tv_post);
	DUMP_TIME_DELTA("exmoore");

	if (!r) {
		goto cleanup;
	}

	/* consolidate */
	TIME(tv_pre);
	dst = fsm_consolidate(fsm, mapping, fsm->statecount);
	TIME(tv_post);
	DUMP_TIME_DELTA("consolidate");

	(void)min_brz;

	if (dst == NULL) {
		r = 0;
		goto cleanup;
	}

	(void)minimised_states;

	fsm_move(fsm, dst);

cleanup:
	if (mapping != NULL) {
		f_free(fsm->opt->alloc, mapping);
	}

	return r;


#else
#if MIN_ALT
	int r = 0;
	size_t states_before, states_naive,
	    states_moore, states_exmoore,
	    states_hopcroft, states_brz;
	fsm_state_t *mapping = NULL;
	const struct fsm_alloc *alloc = fsm->opt ? fsm->opt->alloc : NULL;
	unsigned char labels[256];
	size_t label_count;

#if DUMP_TIME
	struct timeval tv_pre, tv_post;

#define TIME(T) if (0 != gettimeofday(&T, NULL)) { assert(0); }
#define DUMP_TIME_DELTA(NAME)				\
	fprintf(stderr, "%-8s %.3f msec\n", NAME,	\
	    1000.0 * (tv_post.tv_sec - tv_pre.tv_sec)	\
	    + (tv_post.tv_usec - tv_pre.tv_usec)/1000.0);
#else
#define TIME(T)
#define DUMP_TIME_DELTA(NAME)
#endif

	assert(fsm != NULL);

	/* This should only be called with a DFA. */
	assert(fsm_all(fsm, fsm_isdfa));

	if (fsm_trim(fsm, FSM_TRIM_START_AND_END_REACHABLE) < 0) {
		return 0;
	}

	if (fsm->statecount == 0) {
		return 1;	/* empty -- no-op */
	}

	mapping = f_malloc(alloc,
	    fsm->statecount * sizeof(mapping[0]));
	if (mapping == NULL) {
		goto cleanup;
	}

	collect_labels(fsm, labels, &label_count);
	if (label_count == 0) {
		return 1;
	}

	states_before = fsm->statecount;

	assert(fsm->statecount > 0);

	TIME(tv_pre);
	r = min_naive(fsm, labels, label_count, mapping, &states_naive);
	TIME(tv_post);
	DUMP_TIME_DELTA("naive");

	if (!r) {
		goto cleanup;
	}

	assert(fsm->statecount > 0);

	TIME(tv_pre);
	r = min_moore(fsm, labels, label_count, mapping, &states_moore);
	TIME(tv_post);
	DUMP_TIME_DELTA("moore");

	if (!r) {
		goto cleanup;
	}

	TIME(tv_pre);
	r = min_exmoore(fsm, labels, label_count, mapping, &states_exmoore);
	TIME(tv_post);
	DUMP_TIME_DELTA("exmoore");

	if (!r) {
		goto cleanup;
	}

#if DO_HOPCROFT
	TIME(tv_pre);
	r = min_hopcroft(fsm, labels, label_count, mapping, &states_hopcroft);
	TIME(tv_post);
	DUMP_TIME_DELTA("hopcroft");
#else
	states_hopcroft = 0;
#endif


	if (!r) {
		goto cleanup;
	}


	TIME(tv_pre);
	r = min_brz(fsm, &states_brz);
	TIME(tv_post);
	DUMP_TIME_DELTA("brz");

	fprintf(stdout, "# before: %lu, naive: %lu, moore: %lu, exmoore: %lu, hopcroft: %lu, brz: %lu\n",
	    states_before, states_naive, states_moore, states_exmoore, states_hopcroft, states_brz);
	fflush(stdout);

	assert(states_naive <= states_brz);
	assert(states_moore == states_naive);
	assert(states_exmoore == states_moore);

#if DO_HOPCROFT
	assert(states_hopcroft == states_moore);
#endif

cleanup:
	if (mapping != NULL) {
		f_free(alloc, mapping);
	}

	return r;
#else
	assert(fsm != NULL);

	/* This should only be called with a DFA. */
	assert(fsm_all(fsm, fsm_isdfa));

	return min_brz(fsm, NULL);
#endif
#endif
}

/* Brzozowski's algorithm. Destructively modifies the input. */
static int
min_brz(struct fsm *fsm, size_t *minimized_state_count)
{
	int r = fsm_reverse(fsm);
	if (!r) {
		return 0;
	}

	r = fsm_determinise(fsm);
	if (!r) {
		return 0;
	}

	r = fsm_reverse(fsm);
	if (!r) {
		return 0;
	}

	r = fsm_determinise(fsm);
	if (!r) {
		return 0;
	}

	if (minimized_state_count != NULL) {
		*minimized_state_count = fsm->statecount;
	}

	return 1;
}


#if MIN_ALT

/* Naive O(N^2) minimization algorithm, as documented in ... */
static int
min_naive(const struct fsm *fsm,
    const unsigned char *labels, size_t label_count,
    fsm_state_t *mapping, size_t *minimized_state_count)
{
	int res = 0;
	unsigned i, j, label_i;
	int iter, changed;
	fsm_state_t new_id;
	const struct fsm_alloc *alloc = fsm->opt ? fsm->opt->alloc : NULL;
	unsigned *table = NULL;
	fsm_state_t *tmp_map = NULL;
	const fsm_state_t dead_state = fsm->statecount;
	const size_t table_states = (fsm->statecount + 1 /* dead state */);
	const size_t row_words = table_states/(8*sizeof(unsigned))
	    + 1 /* round up */;

#define POS(X,Y) ((X*table_states) + Y)
#define BITS (8*sizeof(unsigned))
#define BIT_OP(X,Y,OP) (table[POS(X,Y)/BITS] OP (1UL << (POS(X,Y) & (BITS - 1))))
#define MARK(X,Y) (BIT_OP(X, Y, |=))
#define CLEAR(X,Y) (BIT_OP(X, Y, &=~))
#define CHECK(X,Y) (BIT_OP(X, Y, &))

	/* This implementation assumes that BITS is a power of 2,
	 * to avoid the extra overhead from modulus. True when
	 * sizeof(unsigned) is 4 or 8. */
	assert((BITS & (BITS - 1)) == 0);

	/* Allocate an NxN bit table, used to track which states have
	 * not yet been proven indistinguishable.
	 *
	 * For two states A and B, only use the half of the table where
	 * A <= B, to avoid redundantly comparing B with A. In other
	 * words, only the bits below the diagonal are actually used.
	 *
	 * An extra state is added to internally represent a complete
	 * graph -- for any state that does not have a particular label
	 * used elsewhere, treat it as if it has that label leading to
	 * an extra dead state. */
	table = f_calloc(alloc,
	    row_words * table_states, sizeof(table[0]));
	if (table == NULL) { goto cleanup; }

	tmp_map = f_malloc(alloc,
	    fsm->statecount * sizeof(tmp_map[0]));
	if (tmp_map == NULL) { goto cleanup; }

	/* Mark all states as potentially indistinguishable from
	 * themselves, as well as any pairs of states that are both
	 * final or not final. */
	for (i = 0; i < fsm->statecount; i++) {
		MARK(i, i);
		/* fprintf(stderr, "-- MARK %u, %u\n", i, i); */
		for (j = 0; j < i; j++) {
			if (fsm_isend(fsm, i) == fsm_isend(fsm, j)) {
				/* fprintf(stderr, "-- MARK %u, %u: 1\n", i, j); */
				MARK(i, j);
			} else {
				/* fprintf(stderr, "-- MARK %u, %u: 0\n", i, j); */
			}
		}
	}
	MARK(dead_state, dead_state);
	/* fprintf(stderr, "-- MARK %u, %u\n", dead_state, dead_state); */

	changed = 1;
	iter = 0;
	while (changed) {	/* run until reaching a fixpoint */
		changed = 0;

#if NAIVE_DUMP_TABLE
		{
			printf("==== iter %d\n", iter);
			for (i = 0; i < table_states-1; i++) {
				for (j = 0; j <= i; j++) {
					printf("%s%d",
					    j > 0 ? " " : "",
					    CHECK(i, j) ? 1 : 0);
				}
				printf("\n");
			}
		}
#endif

		/* For every combination of states that are still
		 * considered potentially indistinguishable, check
		 * whether the current label leads both to a pair
		 * of distinguishable states. If so, then they are
		 * no longer distinguishable (transitively), so mark
		 * them and set changed to note that another iteration
		 * may be necessary. */
		for (i = 0; i < table_states; i++) {
			for (j = 0; j < i; j++) {
				if (!CHECK(i, j)) { continue; }
				for (label_i = 0; label_i < label_count; label_i++) {
					const unsigned char label = labels[label_i];
					const fsm_state_t to_i = naive_delta(fsm, i, label);
					const fsm_state_t to_j = naive_delta(fsm, j, label);
					int tbval;
#if NAIVE_LOG_PARTITION
					fprintf(stderr, "-- label '%c', %u -> %d, %u -> %d\n", label, i, to_i, j, to_j);
#endif

					if (to_i >= to_j) {
						tbval = 0 != CHECK(to_i, to_j);
					} else {
						tbval = 0 != CHECK(to_j, to_i);
					}
					/* fprintf(stderr, "%d,%d -> %d ; %d -> %d\n",
					 *     i, j, to_i, to_j, tbval); */
					if (!tbval) {
#if NAIVE_LOG_PARTITION
						fprintf(stderr, "-- CLEAR: '%c', %u -> %d, %u -> %d\n", label, i, to_i, j, to_j);
#endif
						CLEAR(i, j);
						changed = 1;
						break;
					}
				}
			}
		}

		iter++;
	}

	/* Initialize to (NO_ID) */
	for (i = 0; i < fsm->statecount; i++) {
		tmp_map[i] = NO_ID;
	}

	/* For any state that cannot be proven distinguishable from an
	 * earlier state, map it back to the earlier one. */
	for (i = 1; i < fsm->statecount; i++) {
		for (j = 0; j < i; j++) {
			if (CHECK(i, j)) {
				tmp_map[i] = j;
				/* fprintf(stderr, "# HIT %d,%d\n", i, j); */
			}
		}
	}

	/* Set IDs for the states, mapping some back to earlier states
	 * (combining them). */
	new_id = 0;
	for (i = 0; i < fsm->statecount; i++) {
		if (tmp_map[i] != (fsm_state_t)-1) {
			mapping[i] = mapping[tmp_map[i]];
		} else {
			mapping[i] = new_id;
			new_id++;
		}
	}

	if (minimized_state_count != NULL) {
		*minimized_state_count = new_id;
#if DUMP_MAPPINGS
		for (i = 0; i < *minimized_state_count; i++) {
			fprintf(stderr, "# naive_mapping[%u]: %u\n",
			    i, mapping[i]);
		}
#endif
	}

#if DUMP_STEPS
	fprintf(stderr, "# done in %d step(s), %ld -> %d states\n",
            iter, fsm->statecount, new_id);
#endif

	res = 1;
#undef POS
#undef BITS
#undef MARK
#undef CHECK
#undef CLEAR
#undef BIT_OP

cleanup:
	if (table != NULL) {
		f_free(alloc, table);
	}
	if (tmp_map != NULL) {
		f_free(alloc, tmp_map);
	}
	return res;
}

static fsm_state_t
naive_delta(const struct fsm *fsm, fsm_state_t id, unsigned char label)
{
	struct fsm_edge e;
	struct edge_iter ei;

	assert(id < fsm->statecount);

	for (edge_set_reset(fsm->states[id].edges, &ei);
	     edge_set_next(&ei, &e); ) {
		assert(e.state < fsm->statecount);
		if (e.symbol == label) {
			return e.state;
		}
		/* could skip rest if ordered */
	}

	return fsm->statecount;	/* dead end */
}

/* Implementation of Moore's DFA minimization algorithm,
 * which is similar to Hopcroft's, but iterates to a fixpoint
 * rather than using a queue to eliminate redundant work. */
static int
min_moore(const struct fsm *fsm,
    const unsigned char *labels, size_t label_count,
    fsm_state_t *mapping, size_t *minimized_state_count)
{
	struct moore_env env;
	int changed, res = 0;
	size_t i, iter = 0, steps = 0;
	fsm_state_t ec_count, ec_offset;

	/* Alloc for each state, plus the dead state. */
	size_t alloc_size = (fsm->statecount + 1) * sizeof(env.state_ecs[0]);

	/* Note that the ordering here is significant -- the
	 * not-final EC may be skipped below. */
	enum init_ec { INIT_EC_NOT_FINAL, INIT_EC_FINAL };

	env.fsm = fsm;

	/* This algorithm assumes the DFA is complete; if not,
	 * then simulate a dead-end state internally and make
	 * all other edges go to it. */
	env.dead_state = fsm->statecount;

	/* Data model: There are 1 or more equivalence classes
	 * (hereafter, ECs) initially, which represent all the
	 * end states and (if present) all the non-end states.
	 * If there are no end states, the graph is already
	 * empty, so this shouldn't run.
	 *
	 * The algorithm starts with all states divided into
	 * these initial ECs, and as it detects different results
	 * within an EC (i.e., for a given label, one state leads
	 * to EC #2 but another state to EC #3) it partitions
	 * them further, until it reaches a fixpoint.
	 *
	 * ECs[i] contains the state ID for the first state in
	 * the linked list, then jump[id] contains the next
	 * successive links, or NO_ID for the end of the list.
	 * state_ecs[id] contains the current EC group.
	 * In other words, state_ecs[ID] = EC, and ecs[EC] = X,
	 * where X is either ID or jump[X], ... contains ID.
	 * Ordering does not matter within the list. */
	env.state_ecs = NULL;
	env.jump = NULL;
	env.ecs = NULL;
	ec_count = 0;

	assert(fsm->statecount != 0);
	env.state_ecs = f_malloc(fsm->opt->alloc, alloc_size);
	if (env.state_ecs == NULL) { goto cleanup; }

	env.jump = f_malloc(fsm->opt->alloc, alloc_size);
	if (env.jump == NULL) { goto cleanup; }

	env.ecs = f_malloc(fsm->opt->alloc, alloc_size);
	if (env.ecs == NULL) { goto cleanup; }

	env.ecs[INIT_EC_NOT_FINAL] = NO_ID;
	env.ecs[INIT_EC_FINAL] = NO_ID;
	ec_count = 2;

	/* build initial lists */
	for (i = 0; i < fsm->statecount; i++) {
		const fsm_state_t ec = fsm_isend(fsm, i)
		    ? INIT_EC_FINAL : INIT_EC_NOT_FINAL;
		env.state_ecs[i] = ec;
		env.jump[i] = env.ecs[ec];
		env.ecs[ec] = i;	/* may replace above */
#if MOORE_DUMP_INIT
		fprintf(stderr, "# --init[%lu]: ec %d, jump[]= %d\n", i, ec, env.jump[i]);
#endif
	}

	env.state_ecs[env.dead_state] = NO_ID;

#if MOORE_DUMP_INIT
	for (i = 0; i < ec_count; i++) {
		fprintf(stderr, "# --ec[%lu]: %d\n", i, env.ecs[i]);
	}
#endif

	changed = 1;
	while (changed) {	/* fixpoint */
		size_t l_i, ec_i, cur_ec_count;
		changed = 0;

#if MOORE_DUMP_ECS
		i = 0;
		fprintf(stderr, "# iter %ld, steps %lu\n", iter, steps);
		while (i < ec_count) {
			fprintf(stderr, "M_EC[%lu]:", i);
			moore_dump_ec(stderr, env.ecs[i], env.jump);
			fprintf(stderr, "\n");
			i++;
		}
#else
		(void)moore_dump_ec;
#endif

		/* This loop can add new ECs, but they should not
		 * be counted until the next pass. */
		cur_ec_count = ec_count;

		for (ec_i = 0; ec_i < cur_ec_count; ec_i++) {
			for (l_i = 0; l_i < label_count; l_i++) {
				steps++;
				if (moore_try_partition(&env, labels[l_i],
					ec_i, ec_count)) {
					changed = 1;
					ec_count++;
				}
			}
		}
		iter++;
	}

	/* When all of the original input states were final, then
	 * the not-final EC will be empty. Skip it in the mapping,
	 * otherwise an unnecessary state will be created for it. */
	ec_offset = (env.ecs[INIT_EC_NOT_FINAL] == NO_ID) ? 1 : 0;

	/* Build map condensing to unique states */
	for (i = ec_offset; i < ec_count; i++) {
		fsm_state_t cur = env.ecs[i];
		while (cur != NO_ID) {
			mapping[cur] = i - ec_offset;
			cur = env.jump[cur];
		}
	}

	if (minimized_state_count != NULL) {
		*minimized_state_count = ec_count - ec_offset;
#if DUMP_MAPPINGS
		for (i = 0; i < *minimized_state_count; i++) {
			fprintf(stderr, "# moore_mapping[%lu]: %u\n",
			    i, mapping[i]);
		}
#endif
	}

#if DUMP_STEPS
	fprintf(stderr, "# done in %lu iteration(s), %lu step(s), %ld -> %lu states, label_count %lu\n",
            iter, steps, fsm->statecount, *minimized_state_count, label_count);
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
moore_dump_ec(FILE *f, fsm_state_t start, const fsm_state_t *jump)
{
	fsm_state_t cur = start;
	while (cur != NO_ID) {
		fprintf(f, " %u", cur);
		cur = jump[cur];
	}
}

static fsm_state_t
moore_delta(const struct fsm *fsm, fsm_state_t id, unsigned char label)
{
	struct fsm_edge e;
	struct edge_iter ei;

	assert(id < fsm->statecount);

	for (edge_set_reset(fsm->states[id].edges, &ei);
	     edge_set_next(&ei, &e); ) {
		assert(e.state < fsm->statecount);
		if (e.symbol == label) {
			return e.state;
		}
		/* could skip rest if ordered */
	}

	return fsm->statecount;	/* dead end */
}

static int
moore_try_partition(struct moore_env *env, unsigned char label,
    fsm_state_t ec_src, fsm_state_t ec_dst)
{
	size_t first_count, other_count = 0;
	fsm_state_t cur = env->ecs[ec_src];
	fsm_state_t to, to_ec, first_ec, prev;

#if EXPENSIVE_INTEGRITY_CHECKS
	size_t state_count = 0, psrc_count, pdst_count;
	while (cur != NO_ID) {
		cur = env->jump[cur];
		state_count++;
	}
	cur = env->ecs[ec_src];
#endif

	/* Don't attempt to partition ECs with <2 states. */
	if (cur == NO_ID || env->jump[cur] == NO_ID) { return 0; }

#if MOORE_LOG_PARTITION
	fprintf(stderr, "# --- mtp: checking '%c' for %u\n", label, ec_src);
#endif

	to = moore_delta(env->fsm, cur, label);
	first_ec = env->state_ecs[to];

#if MOORE_LOG_PARTITION
	fprintf(stderr, "# --- mtp: first, %u, has to %d -> first_ec %d\n", cur, to, first_ec);
#endif

	first_count = 1;
	prev = cur;
	cur = env->jump[cur];

	/* initialize the new EC -- empty */
	env->ecs[ec_dst] = NO_ID;

	while (cur != NO_ID) {
		to = moore_delta(env->fsm, cur, label);
		to_ec = env->state_ecs[to];
#if MOORE_LOG_PARTITION
		fprintf(stderr, "# --- mtp: next, cur %u, has to %d -> to_ec %d\n", cur, to, to_ec);
#endif

		if (to_ec == first_ec) { /* in same EC */
			first_count++;
			prev = cur;
			cur = env->jump[cur];
		} else {	/* unlink, split */
			/* Unlink this state from the current EC,
			 * instead put it as the start of a new one
			 * at ecs[ec_dst]. */
			fsm_state_t next = env->jump[cur];

#if MOORE_LOG_PARTITION
			fprintf(stderr, "# mtp: unlinking -- label '%c', src %u, dst %u, first_ec %d, cur %u -> to_ec %d\n", label, ec_src, ec_dst, first_ec, cur, to_ec);
#endif

			env->jump[prev] = next;
			env->jump[cur] = env->ecs[ec_dst];
			env->ecs[ec_dst] = cur;
			cur = next;
			other_count++;
		}
	}

	/* If some were split off, update their state_ecs entries.
	 * Wait to update until after they have all been compared,
	 * otherwise the mix of updated and non-updated states can
	 * lead to inconsistent results. */
	if (other_count > 0) {
		assert(env->ecs[ec_dst] != NO_ID);
		cur = env->ecs[ec_dst];
		while (cur != NO_ID) {
			env->state_ecs[cur] = ec_dst;
			cur = env->jump[cur];
		}
	}

#if EXPENSIVE_INTEGRITY_CHECKS
	psrc_count = 0;
	cur = env->ecs[ec_src];
	while (cur != NO_ID) {
		cur = env->jump[cur];
		psrc_count++;
	}

	pdst_count = 0;
	cur = env->ecs[ec_dst];
	while (cur != NO_ID) {
		cur = env->jump[cur];
		pdst_count++;
	}

	assert(state_count == psrc_count + pdst_count);
#endif

	return other_count > 0;
}
#endif





#if MIN_ALT || MIN_REAL
/* We only know the count for how many states are in an
 * EC after an attempted partition. When the ECs have
 * less than EXMOORE_EC_SMALL_THRESHOLD states, then
 * set the upper bit to mark that it should get smarter
 * handling -- instead of trying every label, look at what
 * labels actually appear in the group, and then just try
 * those, possibly guided by counts. */
#define EXMOORE_EC_MSB (UINT_MAX ^ (UINT_MAX >> 1))
#define EXMOORE_EC_SMALL_THRESHOLD 16
#define EXMOORE_EC_LABELS_THRESHOLD 4
#define EXMOORE_BOX_EC_HEAD(STATE_ID, IS_SMALL)	\
	(STATE_ID | (IS_SMALL ? EXMOORE_EC_MSB : 0))
#define EXMOORE_UNBOX_EC_HEAD(EC)		\
	(EC &~ EXMOORE_EC_MSB)

static void
exmoore_update_ec(struct exmoore_env *env, fsm_state_t ec_dst)
{
	size_t count = 0;
	fsm_state_t cur = env->ecs[ec_dst];

	assert(cur != NO_ID);
	while (cur != NO_ID) {
		cur = EXMOORE_UNBOX_EC_HEAD(cur);
		env->state_ecs[cur] = ec_dst;
		cur = env->jump[cur];
		count++;
	}

	/* Flag the EC for smarter label checking */
	if (count <= EXMOORE_EC_SMALL_THRESHOLD
		&& env->label_count >= EXMOORE_EC_LABELS_THRESHOLD) {
		env->ecs[ec_dst] = EXMOORE_BOX_EC_HEAD(env->ecs[ec_dst], 1);
	}
}

/* EXperimental, EXtended version of Moore's. */
static int
min_exmoore(const struct fsm *fsm,
    const unsigned char *labels, size_t label_count,
    fsm_state_t *mapping, size_t *minimized_state_count)
{
	struct exmoore_env env;
	int changed, res = 0;
	size_t i, iter = 0, steps = 0;

	fsm_state_t ec_count, ec_offset;
	fsm_state_t done_ec_offset;

	/* Alloc for each state, plus the dead state. */
	size_t alloc_size = (fsm->statecount + 1) * sizeof(env.state_ecs[0]);

	/* Note that the ordering here is significant -- the
	 * not-final EC may be skipped below. */
	enum init_ec { INIT_EC_NOT_FINAL, INIT_EC_FINAL };

	env.fsm = fsm;

	/* This algorithm assumes the DFA is complete; if not,
	 * then simulate a dead-end state internally and make
	 * all other edges go to it. */
	env.dead_state = fsm->statecount;

	/* Data model: There are 1 or more equivalence classes
	 * (hereafter, ECs) initially, which represent all the
	 * end states and (if present) all the non-end states.
	 * If there are no end states, the graph is already
	 * empty, so this shouldn't run.
	 *
	 * The algorithm starts with all states divided into
	 * these initial ECs, and as it detects different results
	 * within an EC (i.e., for a given label, one state leads
	 * to EC #2 but another state to EC #3) it partitions
	 * them further, until it reaches a fixpoint.
	 *
	 * ECs[i] contains the state ID for the first state in
	 * the linked list, then jump[id] contains the next
	 * successive links, or NO_ID for the end of the list.
	 * state_ecs[id] contains the current EC group.
	 * In other words, state_ecs[ID] = EC, and ecs[EC] = X,
	 * where X is either ID or jump[X], ... contains ID.
	 * Ordering does not matter within the list. */
	env.state_ecs = NULL;
	env.jump = NULL;
	env.ecs = NULL;
	env.labels = labels;
	env.label_count = label_count;
	ec_count = 0;

	assert(fsm->statecount != 0);
	env.state_ecs = f_malloc(fsm->opt->alloc, alloc_size);
	if (env.state_ecs == NULL) { goto cleanup; }

	env.jump = f_malloc(fsm->opt->alloc, alloc_size);
	if (env.jump == NULL) { goto cleanup; }

	env.ecs = f_malloc(fsm->opt->alloc, alloc_size);
	if (env.ecs == NULL) { goto cleanup; }

	env.ecs[INIT_EC_NOT_FINAL] = NO_ID;
	env.ecs[INIT_EC_FINAL] = NO_ID;
	ec_count = 2;
	done_ec_offset = ec_count;

	/* build initial lists */
	for (i = 0; i < fsm->statecount; i++) {
		const fsm_state_t ec = fsm_isend(fsm, i)
		    ? INIT_EC_FINAL : INIT_EC_NOT_FINAL;
		env.state_ecs[i] = ec;
		env.jump[i] = env.ecs[ec];
		env.ecs[ec] = i;	/* may replace above */
#if EXMOORE_DUMP_INIT
		fprintf(stderr, "# --init[%lu]: ec %d, jump[]= %d\n", i, ec, env.jump[i]);
#endif
	}

	env.state_ecs[env.dead_state] = NO_ID;

#if EXMOORE_DUMP_INIT
	for (i = 0; i < ec_count; i++) {
		fprintf(stderr, "# --ec[%lu]: %d\n", i, env.ecs[i]);
	}
#endif

	exmoore_heuristic_scan(stderr, &env, labels, label_count);

	changed = 1;
	while (changed) {	/* fixpoint */
		size_t l_i, ec_i;
		changed = 0;

#if EXMOORE_DUMP_ECS
		i = 0;
		fprintf(stderr, "# iter %ld, steps %lu\n", iter, steps);
		while (i < ec_count) {
			fprintf(stderr, "M_EC[%lu]:", i);
			exmoore_dump_ec(stderr,
			    EXMOORE_UNBOX_EC_HEAD(env.ecs[i]),
			    env.jump);
			fprintf(stderr, "\n");
			i++;
		}
#else
		(void)exmoore_dump_ec;
#endif

		for (ec_i = 0; ec_i < done_ec_offset; ec_i++) {

			/* Don't bother checking any ECs with less than two
			 * states (they can't be split further), and note
			 * if the flag was set for collecting the EC's edges. */
			int special_handling;
			struct exmoore_label_iterator li;
	restart_EC:
			{
				fsm_state_t next = env.ecs[ec_i];
				if (next == NO_ID) { continue; }
				special_handling = next & EXMOORE_EC_MSB;
				next = EXMOORE_UNBOX_EC_HEAD(next);

				next = env.jump[next];
				if (next == NO_ID) { continue; }
			}

			exmoore_init_label_iterator(&env, ec_i,
			    special_handling, &li);

			while (li.i < li.limit) {
				size_t pcounts[2];
				const fsm_state_t ec_src = ec_i;
				const fsm_state_t ec_dst = ec_count;
				unsigned char label;

				l_i = li.i;
				li.i++;

				label = (li.use_special
				    ? li.labels[l_i]
				    : labels[l_i]);

				steps++;
				if (exmoore_try_partition(&env, label,
					ec_src, ec_dst, pcounts)) {

#if EXMOORE_LOG_PARTITION
					fprintf(stderr, "# partition: ec_i %lu/%u/%u, l_i %lu/%u%s, pcounts [ %lu, %lu ]\n",
					    ec_i, done_ec_offset, ec_count,
					    l_i, li.limit, li.use_special ? "*" : "",
					    pcounts[0], pcounts[1]);
#endif

					if (pcounts[1] == 1) {
						assert(done_ec_offset > 0);
						exmoore_update_ec(&env, ec_dst);
					} else {
						/* swap so that dst is before the done group */
						const fsm_state_t ndst = done_ec_offset;
						const fsm_state_t tmp = env.ecs[ec_dst];
						env.ecs[ec_dst] = env.ecs[ndst];
						env.ecs[ndst] = tmp;
						exmoore_update_ec(&env, ec_dst);
						exmoore_update_ec(&env, ndst);
						done_ec_offset++;
					}

					/* if the src only has one, swap it into the done group */
					if (pcounts[0] == 1 && done_ec_offset > 0) { /* swap */
						fsm_state_t nsrc = done_ec_offset - 1;

						/* swap ec[nsrc] and ec[ec_src] */
						const fsm_state_t tmp = env.ecs[ec_src];
						env.ecs[ec_src] = env.ecs[nsrc];
						env.ecs[nsrc] = tmp;
						exmoore_update_ec(&env, ec_src);
						exmoore_update_ec(&env, nsrc);

						assert(done_ec_offset > 0);
						done_ec_offset--;

						/* The src EC just got swapped with another,
						 * so start over at the beginning for the new EC. */
						changed = 1;
						ec_count++;
						goto restart_EC;
					}

					changed = 1;
					ec_count++;

					assert(done_ec_offset <= ec_count);
				}

#if EXPENSIVE_INTEGRITY_CHECKS
				for (i = 0; i < ec_count; i++) {
					const fsm_state_t head = EXMOORE_UNBOX_EC_HEAD(env.ecs[i]);
					if (i >= done_ec_offset) {
						assert(head == NO_ID || env.jump[head] == NO_ID);
					} else {
						assert(env.jump[head] != NO_ID);
					}
				}
#endif
			}
		}

		iter++;
	}

	/* When all of the original input states were final, then
	 * the not-final EC will be empty. Skip it in the mapping,
	 * otherwise an unnecessary state will be created for it. */
	ec_offset = (env.ecs[INIT_EC_NOT_FINAL] == NO_ID) ? 1 : 0;

	/* Build map condensing to unique states */
	for (i = ec_offset; i < ec_count; i++) {
		fsm_state_t cur = EXMOORE_UNBOX_EC_HEAD(env.ecs[i]);
		while (cur != NO_ID) {
			mapping[cur] = i - ec_offset;
			cur = env.jump[cur];
		}
	}

	if (minimized_state_count != NULL) {
		*minimized_state_count = ec_count - ec_offset;
#if DUMP_MAPPINGS
		for (i = 0; i < fsm->statecount; i++) {
			fprintf(stderr, "# exmoore_mapping[%lu]: %u\n",
			    i, mapping[i]);
		}
#endif
	}

#if DUMP_STEPS
	fprintf(stderr, "# done in %lu iteration(s), %lu step(s), %ld -> %lu states, label_count %lu\n",
            iter, steps, fsm->statecount, *minimized_state_count, label_count);
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
exmoore_init_label_iterator(const struct exmoore_env *env,
	fsm_state_t ec_i, int special_handling,
	struct exmoore_label_iterator *li)
{
	/* If the MSB flag was set for the EC (indicating that it's fairly
	 * small), then instead of trying every label, save time by only
	 * checking the labels that are actually present. */
	if (special_handling) {
		fsm_state_t cur;
		size_t i;
		uint32_t label_set[8];
		memset(label_set, 0x00, sizeof(label_set));

		cur = env->ecs[ec_i];
		assert(cur != NO_ID);
		cur = EXMOORE_UNBOX_EC_HEAD(cur);

		while (cur != NO_ID) {
			struct fsm_edge e;
			struct edge_iter ei;
			for (edge_set_reset(env->fsm->states[cur].edges, &ei);
			     edge_set_next(&ei, &e); ) {
				const unsigned char label = e.symbol;
				label_set[label/32] |= ((uint32_t)1 << (label & 31));
			}
			cur = env->jump[cur];
		}

		li->use_special = 1;
		li->i = 0;
		li->limit = 0;
		i = 0;
		while (i < 256) {
			if ((i & 31) == 0 && label_set[i/32] == 0) {
				i += 32;
			} else {
				if (label_set[i/32] & ((uint32_t)1 << (i & 31))) {
					li->labels[li->limit] = i;
					li->limit++;
				}
				i++;
			}
		}
	} else {		/* all labels in sequence */
		li->use_special = 0;
		li->limit = env->label_count;
		li->i = 0;
	}
}

static void
exmoore_dump_ec(FILE *f, fsm_state_t start, const fsm_state_t *jump)
{
	fsm_state_t cur = start;
	while (cur != NO_ID) {
		fprintf(f, " %u", cur);
		cur = jump[cur];
	}
}

static fsm_state_t
exmoore_delta(const struct fsm *fsm, fsm_state_t id, unsigned char label)
{
	struct fsm_edge e;
	struct edge_iter ei;

	assert(id < fsm->statecount);

	/* FIXME: use edge_set_find */
	for (edge_set_reset(fsm->states[id].edges, &ei);
	     edge_set_next(&ei, &e); ) {
		assert(e.state < fsm->statecount);
		if (e.symbol == label) {
			return e.state;
		}
		/* could skip rest if ordered */
	}

	return fsm->statecount;	/* dead end */
}

/* unlike moore above:
 * any split off get linked into EC_DST, but don't update the
 * state_ec and other fields, because depending on how many are
 * extracted, they may get swapped. */
static int
exmoore_try_partition(struct exmoore_env *env, unsigned char label,
    fsm_state_t ec_src, fsm_state_t ec_dst,
	size_t partition_counts[2])
{
	fsm_state_t cur = EXMOORE_UNBOX_EC_HEAD(env->ecs[ec_src]);
	fsm_state_t to, to_ec, first_ec, prev;

#if EXPENSIVE_INTEGRITY_CHECKS
	size_t state_count = 0, psrc_count, pdst_count;
	while (cur != NO_ID) {
		cur = env->jump[cur];
		state_count++;
	}
	cur = EXMOORE_UNBOX_EC_HEAD(env->ecs[ec_src]);
#endif

	memset(partition_counts, 0x00, 2*sizeof(partition_counts[0]));

#if EXMOORE_LOG_PARTITION
	fprintf(stderr, "# --- mtp: checking '%c' for %u\n", label, ec_src);
#endif

	to = exmoore_delta(env->fsm, cur, label);
	first_ec = env->state_ecs[to];

#if EXMOORE_LOG_PARTITION
	fprintf(stderr, "# --- mtp: first, %u, has to %d -> first_ec %d\n", cur, to, first_ec);
#endif

	partition_counts[0] = 1;
	prev = cur;
	cur = env->jump[cur];

	/* initialize the new EC -- empty */
	env->ecs[ec_dst] = NO_ID;

	while (cur != NO_ID) {
		to = exmoore_delta(env->fsm, cur, label);
		to_ec = env->state_ecs[to];
#if EXMOORE_LOG_PARTITION
		fprintf(stderr, "# --- mtp: next, cur %u, has to %d -> to_ec %d\n", cur, to, to_ec);
#endif

		if (to_ec == first_ec) { /* in same EC */
			partition_counts[0]++;
			prev = cur;
			cur = env->jump[cur];
		} else {	/* unlink, split */
			/* Unlink this state from the current EC,
			 * instead put it as the start of a new one
			 * at ecs[ec_dst]. */
			fsm_state_t next = env->jump[cur];

#if EXMOORE_LOG_PARTITION
			fprintf(stderr, "# mtp: unlinking -- label '%c', src %u, dst %u, first_ec %d, cur %u -> to_ec %d\n", label, ec_src, ec_dst, first_ec, cur, to_ec);
#endif

			env->jump[prev] = next;
			env->jump[cur] = env->ecs[ec_dst];
			env->ecs[ec_dst] = cur;
			cur = next;
			partition_counts[1]++;
		}
	}

#if EXPENSIVE_INTEGRITY_CHECKS
	psrc_count = 0;
	cur = env->ecs[ec_src];
	if (cur != NO_ID) {
		cur = EXMOORE_UNBOX_EC_HEAD(cur);
		while (cur != NO_ID) {
			cur = env->jump[cur];
			psrc_count++;
		}
	}

	pdst_count = 0;
	cur = env->ecs[ec_dst];
	if (cur != NO_ID) {
		cur = EXMOORE_UNBOX_EC_HEAD(cur);
		while (cur != NO_ID) {
			cur = env->jump[cur];
			pdst_count++;
		}
	}

	assert(state_count == psrc_count + pdst_count);
#endif

	return partition_counts[1] > 0;
}

static void
exmoore_heuristic_scan(FILE *f, const struct exmoore_env *env,
    const unsigned char *labels, size_t label_count)
{
#if 0
	size_t i, s_i, e_i;
	const struct fsm *fsm = env->fsm;
	uint8_t label_map[256];
	size_t *label_counts = calloc(label_count, sizeof(label_counts[0]));
	assert(label_counts != NULL);

	return;

	for (i = 0; i < label_count; i++) {
		label_map[labels[i]] = i;
	}

	for (s_i = 0; s_i < fsm->statecount; s_i++) {
		struct fsm_edge e;
		struct edge_iter ei;
		const size_t edge_count = edge_set_count(fsm->states[s_i].edges);

		e_i = 0;
		for (edge_set_reset(fsm->states[s_i].edges, &ei);
		     edge_set_next(&ei, &e); ) {
			fprintf(f, "state %lu -- edge %lu/%lu -- '%c' -> %u\n",
			    s_i, e_i, edge_count, e.symbol, e.state);
			e_i++;
			label_counts[label_map[e.symbol]]++;
		}
	}

	for (i = 0; i < label_count; i++) {
		fprintf(f, "label '%c': %lu\n", labels[i], label_counts[i]);
	}

	free(label_counts);
#else
	(void)f;
	(void)env;
	(void)labels;
	(void)label_count;
#endif
}
#endif







#if MIN_ALT
#if HOPCROFT_DUMP_INIT
static void
dump_edges(const struct fsm *fsm)
{
	struct fsm_edge e;
	struct edge_iter ei;
	fsm_state_t id;

	for (id = 0; id < fsm->statecount; id++) {
		for (edge_set_reset(fsm->states[id].edges, &ei);
		     edge_set_next(&ei, &e); ) {
			fprintf(stderr, "dump_edges: state[%d], edge <'%c' -> %u>\n",
			    id, e.symbol, e.state);
		}
	}
}
#endif

/* An fsm_state_id with the most significant bit set is
 * an offset to the next cell on the freelist. The freelist
 * is terminated with NO_ID, just like the EC lists. */
#define FREELIST_MSB (UINT_MAX ^ (UINT_MAX >> 1))
#define IS_FREELIST(ID) (ID & FREELIST_MSB)
#define BOX_FREELIST(ID) (ID | FREELIST_MSB)
#define UNBOX_FREELIST(ID) (ID &~ FREELIST_MSB)

#if DO_HOPCROFT
static int
min_hopcroft(const struct fsm *fsm,
    const unsigned char *labels, size_t label_count,
    fsm_state_t *mapping, size_t *minimized_state_count)
{
	/* init splitter stack */
	struct hopcroft_env env;
	int res = 0;
	size_t i, iter = 0, steps = 0;
	fsm_state_t ec_count;

	/* Alloc for each state, plus the dead state. */
	size_t per_state_alloc_size = (fsm->statecount + 1)
	    * sizeof(env.state_ecs[0]);

	/* Note that the ordering here is significant -- the
	 * not-final EC may be skipped below. */
	enum init_ec { INIT_EC_NOT_FINAL, INIT_EC_FINAL };

	memset(&env, 0x00, sizeof(env));

	env.fsm = fsm;
	env.labels = labels;
	env.label_count = label_count;

	/* This algorithm assumes the DFA is complete; if not,
	 * then simulate a dead-end state internally and make
	 * all other edges go to it. */
	env.dead_state = fsm->statecount;

	/* Data model: There are 1 or more equivalence classes
	 * (hereafter, ECs) initially, which represent all the
	 * end states and (if present) all the non-end states.
	 * If there are no end states, the graph is already
	 * empty, so this shouldn't run.
	 *
	 * The algorithm starts with all states divided into
	 * these initial ECs, and as it detects different results
	 * within an EC (i.e., for a given label, one state leads
	 * to EC #2 but another state to EC #3) it partitions
	 * them further, until it reaches a fixpoint.
	 *
	 * ECs[i] contains the state ID for the first state in
	 * the linked list, then jump[id] contains the next
	 * successive links, or NO_ID for the end of the list.
	 * state_ecs[id] contains the current EC group.
	 * In other words, state_ecs[ID] = EC, and ecs[EC] = X,
	 * where X is either ID or jump[X], ... contains ID.
	 * Ordering does not matter within the list. */

	assert(fsm->statecount != 0);

	if (!hopcroft_build_label_index(&env, labels, label_count)) {
		goto cleanup;
	}

	env.stack.splitters = f_malloc(fsm->opt->alloc,
	    DEF_SPLITTER_STACK_CEIL * sizeof(env.stack.splitters[0]));
	if (env.stack.splitters == NULL) { goto cleanup; }
	env.stack.ceil = DEF_SPLITTER_STACK_CEIL;
	env.stack.count = 0;
	for (i = 0; i < 256; i++) {
		env.stack.index[i] = (size_t)-1;
	}

	env.ec_filter_bytes = (fsm->statecount/sizeof(uint64_t) + 1)
	    * sizeof(uint64_t);
	env.ec_filter = f_malloc(fsm->opt->alloc, env.ec_filter_bytes);
	if (env.ec_filter == NULL) { goto cleanup; }

	env.state_ecs = f_malloc(fsm->opt->alloc, per_state_alloc_size);
	if (env.state_ecs == NULL) { goto cleanup; }

	env.jump = f_malloc(fsm->opt->alloc, per_state_alloc_size);
	if (env.jump == NULL) { goto cleanup; }

	/* TODO: make this proportional to the state count */
	env.ecs_ceil = 4;
	env.ecs = f_malloc(fsm->opt->alloc, env.ecs_ceil * sizeof(env.ecs[0]));
	if (env.ecs == NULL) { goto cleanup; }

	env.updated_ecs_ceil = 4;
	env.updated_ecs = f_malloc(fsm->opt->alloc,
	    env.updated_ecs_ceil * sizeof(env.updated_ecs[0]));
	if (env.updated_ecs == NULL) { goto cleanup; }

	/* Build a freelist for available ECs */
	for (i = 0; i < env.ecs_ceil; i++) {
		env.ecs[i] = BOX_FREELIST(i + 1);
	}
	env.ecs[env.ecs_ceil - 1] = NO_ID;

	env.ecs[INIT_EC_NOT_FINAL] = NO_ID;
	env.ecs[INIT_EC_FINAL] = NO_ID;
	env.max_ec_in_use = INIT_EC_FINAL;

	env.ec_freelist = BOX_FREELIST(env.max_ec_in_use + 1);

#if HOPCROFT_DUMP_INIT
	for (i = 0; i < fsm->statecount; i++) {
		fprintf(stderr, "freelist[%lu]: 0x%x\n", i, env.ecs[i]);
	}
#endif

	{
		size_t ecs_counts[2] = { 0, 0 };
		size_t smaller_ec;

		/* build initial lists */
		for (i = 0; i < fsm->statecount; i++) {
			const fsm_state_t ec = fsm_isend(fsm, i)
			    ? INIT_EC_FINAL : INIT_EC_NOT_FINAL;
			env.state_ecs[i] = ec;
			env.jump[i] = env.ecs[ec];
			env.ecs[ec] = i;	/* may replace above */
#if HOPCROFT_DUMP_INIT
			fprintf(stderr, "# --init[%lu]: ec %d, jump[]= %d\n", i, ec, env.jump[i]);
#endif
			ecs_counts[ec]++;

		}

		/* The dead state is an honorary member of the not-final EC.
		 * Only add it if necessary. */
		if (!fsm_all(fsm, fsm_iscomplete)) {
			env.state_ecs[env.dead_state] = INIT_EC_NOT_FINAL;
			env.jump[env.dead_state] = env.ecs[INIT_EC_NOT_FINAL];
			env.ecs[INIT_EC_NOT_FINAL] = env.dead_state;
			ecs_counts[INIT_EC_NOT_FINAL]++;
		}

		if (ecs_counts[INIT_EC_NOT_FINAL] == 0) {
			env.ecs[INIT_EC_NOT_FINAL] = env.ec_freelist;
			env.ec_freelist = BOX_FREELIST(INIT_EC_NOT_FINAL);
		}

		/* populate initial splitters */
		smaller_ec = INIT_EC_FINAL;
		if (ecs_counts[INIT_EC_NOT_FINAL] < ecs_counts[INIT_EC_FINAL]
			&& ecs_counts[INIT_EC_NOT_FINAL] > 0) {
			smaller_ec = INIT_EC_NOT_FINAL;
		}

		for (i = 0; i < label_count; i++) {
			unsigned char label = labels[i];

			if (!hopcroft_schedule_splitter(&env,
				smaller_ec, label)) {
				goto cleanup;
			}
		}
	}

#if HOPCROFT_DUMP_INIT
	dump_edges(env.fsm);
#endif

	while (env.stack.count > 0) { /* until empty */
		size_t l_i, ec_i;
		struct splitter *splitter;
		fsm_state_t splitter_ec;
		unsigned char splitter_label;

		/* FIXME: This should be dynamically allocated,
		 * starting fairly small and growing on demand.
		 * It's probably possible to determine the worst
		 * case for this based on the number of input
		 * states and preallocate it. */
		size_t updated_ecs_i = 0;

#if HOPCROFT_DUMP_INIT
	for (i = 0; i < fsm->statecount; i++) {
		fprintf(stderr, "freelist[%lu]: 0x%x\n", i, env.ecs[i]);
	}
#endif

#if HOPCROFT_DUMP_ECS
		i = 0;
		fprintf(stderr, "# iter %ld, steps %lu\n", iter, steps);
		while (i <= env.max_ec_in_use) {
			if (IS_FREELIST(env.ecs[i])) {
				i++;
				continue;
			}
			fprintf(stderr, "H_EC[%lu]:", i);
			hopcroft_dump_ec(stderr, env.ecs[i], env.jump);
			fprintf(stderr, "\n");
			i++;
		}
#else
		(void)hopcroft_dump_ec;
#endif

#if HOPCROFT_LOG_STACK
		fprintf(stderr, "# stack[%zu]: ", env.stack.count);
		for (i = 0; i < env.stack.count; i++) {
			fprintf(stderr, " <%d,'%c'>",
			    env.stack.splitters[i].ec,
			    env.stack.splitters[i].label);
		}
		fprintf(stderr, "\n");
#endif

		assert(env.stack.count > 0);
		env.stack.count--;
		splitter = &env.stack.splitters[env.stack.count];
		splitter_ec = splitter->ec;
		splitter_label = splitter->label;
		env.stack.index[splitter_label] = splitter->prev;
#if HOPCROFT_LOG_STACK
		fprintf(stderr, "# popped splitter: <%d,'%c'>\n",
		    splitter_ec, splitter_label);
#endif

		hopcroft_mark_ec_filter(&env, splitter_label);

		/* This can add new ECs, but they should not be
		 * considered active until after processing this
		 * splitter. */
		for (ec_i = 0; ec_i <= env.max_ec_in_use; ec_i++) {
			const fsm_state_t ec_src = ec_i;
			fsm_state_t ec_dst_a;
			fsm_state_t ec_dst_b;
			/* size_t l_i; */
			size_t pcs[2]; /* partition counts */
			enum hopcroft_try_partition_res pres;

			if (!hopcroft_check_ec_filter(env.ec_filter, ec_i)) {
				/* fprintf(stderr, "SKIP %u\n", ec_i); */
				continue;
			}

#if !HOPCROFT_USE_LABEL_INDEX
			if (IS_FREELIST(env.ecs[ec_i])) {
				continue;
			}
#endif
			steps++;

			pres = hopcroft_try_partition(&env,
			    splitter_ec, splitter_label,
			    ec_src, &ec_dst_a, &ec_dst_b, pcs);

			if (pres == HTP_ERROR_ALLOC) {
				goto cleanup;
			} else if (pres == HTP_PARTITIONED) {
				struct updated_ecs *updated = &env.updated_ecs[updated_ecs_i];
#if HOPCROFT_LOG_PARTITION
				fprintf(stderr, "# partitioned EC %u into EC %u (w/ %lu) and EC %u (w/ %lu)\n",
				    ec_src,
				    ec_dst_a, pcs[0],
				    ec_dst_b, pcs[1]);
#endif

				/* Note which ECs were updated, for
				 * further bookkeeping after the
				 * splitter pass completes. The dst EC
				 * with fewer states always goes
				 * first. */
				updated->src = ec_src;
				updated->dst[0] = pcs[0] < pcs[1] ? ec_dst_a : ec_dst_b;
				updated->dst[1] = pcs[0] < pcs[1] ? ec_dst_b : ec_dst_a;

				updated_ecs_i++;
				if (updated_ecs_i == env.updated_ecs_ceil) {
					if (!hopcroft_grow_updated_ecs(&env)) {
						goto cleanup;
					}
				}

				/* Box the newly created ECs so that they
				 * will be ignored while doing the remainder
				 * of the pass for this splitter. */
				env.ecs[ec_dst_a] = BOX_FREELIST(env.ecs[ec_dst_a]);
				env.ecs[ec_dst_b] = BOX_FREELIST(env.ecs[ec_dst_b]);
			} else {
				assert(pres == HTP_NO_CHANGE);
			}
		}

		/* Now that the splitter pass is done, update ECs that
		 * were processed by it -- the src EC gets put on the
		 * freelist, the two dst ECs become active. */
		for (ec_i = 0; ec_i < updated_ecs_i; ec_i++) {
			struct updated_ecs *updated = &env.updated_ecs[ec_i];

			/* put the partitioned EC back on the freelist */
			assert(IS_FREELIST(env.ec_freelist));
#if HOPCROFT_LOG_FREELIST
			fprintf(stderr, "putting %u back on the freelist, pointing to 0x%x\n",
			    updated->src, env.ec_freelist);
#endif
			env.ecs[updated->src] = env.ec_freelist;
			env.ec_freelist = BOX_FREELIST(updated->src);

			for (i = 0; i < 2; i++) {
				fsm_state_t cur;
				assert(IS_FREELIST(env.ecs[updated->dst[i]]));
				env.ecs[updated->dst[i]] = UNBOX_FREELIST(env.ecs[updated->dst[i]]);
				if (updated->dst[i] > env.max_ec_in_use) {
					env.max_ec_in_use = updated->dst[i];
				}

				cur = env.ecs[updated->dst[i]];
				while (cur != NO_ID) {
					fsm_state_t next = env.jump[cur];
					env.state_ecs[cur] = updated->dst[i];
					assert(next != cur);
					cur = next;
				}
			}

			/* Schedule further splitters */
			for (l_i = 0; l_i < label_count; l_i++) {
				const unsigned char label = labels[l_i];
				/* dst[0] always has <= states than dst[1], ordered above. */
				fsm_state_t ec_min = updated->dst[0];
				if (!hopcroft_update_splitter_stack(&env,
					label, updated->src,
					updated->dst[0], updated->dst[1], ec_min)) {
					goto cleanup;
				}
			}
		}

		iter++;
	}

	/* Build map condensing to unique states */
	ec_count = 0;
	for (i = 0; i <= env.max_ec_in_use; i++) {
		fsm_state_t cur;
		size_t count = 0;

		if (IS_FREELIST(env.ecs[i])) {
			continue;
		}

		cur = env.ecs[i];
		while (cur != NO_ID) {
			/* Don't include the dead state in the result. */
			if (cur == env.dead_state) {
				cur = env.jump[cur];
				if (cur == NO_ID) {
					break;
				}
			}

			count++;

			mapping[cur] = ec_count;
			cur = env.jump[cur];
		}

		if (count > 0) {
			ec_count++;
		}
	}

	if (minimized_state_count != NULL) {
		*minimized_state_count = ec_count;

#if DUMP_MAPPINGS
		for (i = 0; i < *minimized_state_count; i++) {
			fprintf(stderr, "# hopcroft_mapping[%lu]: %u\n",
			    i, mapping[i]);
		}
#endif
	}

#if DUMP_STEPS
	fprintf(stderr, "# done in %lu iteration(s), %lu step(s), %ld -> %lu states, label_count %lu\n",
            iter, steps, fsm->statecount, *minimized_state_count, label_count);
#endif

	res = 1;
	/* fall through */
cleanup:
	f_free(fsm->opt->alloc, env.ecs);
	f_free(fsm->opt->alloc, env.updated_ecs);
	f_free(fsm->opt->alloc, env.state_ecs);
	f_free(fsm->opt->alloc, env.jump);
	f_free(fsm->opt->alloc, env.stack.splitters);
	f_free(fsm->opt->alloc, env.label_offsets);
	f_free(fsm->opt->alloc, env.label_index);
	f_free(fsm->opt->alloc, env.ec_filter);
	return res;
}

static void
hopcroft_dump_ec(FILE *f, fsm_state_t start, const fsm_state_t *jump)
{
	moore_dump_ec(f, start, jump);
}

static fsm_state_t
hopcroft_delta(const struct fsm *fsm, fsm_state_t id, unsigned char label)
{
	struct fsm_edge e;
	struct edge_iter ei;

	if (id == fsm->statecount) {
		return fsm->statecount;
	}

	assert(id < fsm->statecount);

	for (edge_set_reset(fsm->states[id].edges, &ei);
	     edge_set_next(&ei, &e); ) {
		assert(e.state < fsm->statecount);
		if (e.symbol == label) {
			return e.state;
		} else if (e.symbol > label) {
			break;	/* ordered -- skip */
		}
	}

	return fsm->statecount;	/* dead end */
}

static int
hopcroft_freelist_pop(struct hopcroft_env *env,
    fsm_state_t *id)
{
	assert(IS_FREELIST(env->ec_freelist));
	if (env->ec_freelist == NO_ID) {
		if (!hopcroft_grow_ecs_freelist(env)) {
			return 0;
		}
	}

	assert(env->ec_freelist != NO_ID);

	*id = UNBOX_FREELIST(env->ec_freelist);
	env->ec_freelist = env->ecs[*id];
	return 1;
}

static void
hopcroft_freelist_push(struct hopcroft_env *env,
    fsm_state_t id)
{
	env->ecs[id] = env->ec_freelist;
	env->ec_freelist = BOX_FREELIST(id);
}

static enum hopcroft_try_partition_res
hopcroft_try_partition(struct hopcroft_env *env,
    fsm_state_t ec_splitter, unsigned char label,
    fsm_state_t ec_src,
    fsm_state_t *ec_dst_a, fsm_state_t *ec_dst_b,
    size_t partition_counts[2])
{
	enum hopcroft_try_partition_res res = HTP_NO_CHANGE;

	/* Partition EC[ec_src] into the sets that have edges
	 * with label that lead to EC_SPLITTER and those that
	 * don't. */
	fsm_state_t cur = env->ecs[ec_src];
	fsm_state_t to, to_ec;

	fsm_state_t dst_a, dst_b;

	/* Don't attempt to partition ECs with <2 states. */
	if (cur == NO_ID || env->jump[cur] == NO_ID) {
		return HTP_NO_CHANGE;
	}

#if HOPCROFT_LOG_PARTITION
	fprintf(stderr, "# --- htp: checking '%c' for %u\n", label, ec_src);
#endif

#if HOPCROFT_LOG_FREELIST
	fprintf(stderr, "FL 0x%x\n", env->ec_freelist);
#endif

	if (!hopcroft_freelist_pop(env, &dst_a)) {
		return HTP_ERROR_ALLOC;
	}

	if (!hopcroft_freelist_pop(env, &dst_b)) {
		return HTP_ERROR_ALLOC;
	}

	/* initialize the new ECs -- empty */
	env->ecs[dst_a] = NO_ID;
	env->ecs[dst_b] = NO_ID;

	partition_counts[0] = 0;
	partition_counts[1] = 0;

	while (cur != NO_ID) {
		const fsm_state_t next = env->jump[cur];
		to = hopcroft_delta(env->fsm, cur, label);
		to_ec = env->state_ecs[to];

#if HOPCROFT_LOG_PARTITION
		fprintf(stderr, "# --- htp: cur %u, next %u, has to %d -> to_ec %d\n", cur, next, to, to_ec);
#endif

		if (to_ec == ec_splitter) { /* -> dst_a */
#if HOPCROFT_LOG_PARTITION
			fprintf(stderr, "#    ---> linked to EC %u (a), head was %u now %u\n",
			    dst_a, env->ecs[dst_a], cur);
#endif
			env->jump[cur] = env->ecs[dst_a];
			env->ecs[dst_a] = cur;
			cur = next;
			partition_counts[0]++;
		} else {	/* -> dst_b */
#if HOPCROFT_LOG_PARTITION
			fprintf(stderr, "#    ---> linked to EC %u (b), head was %u now %u\n",
			    dst_b, env->ecs[dst_b], cur);
#endif
			env->jump[cur] = env->ecs[dst_b];
			env->ecs[dst_b] = cur;
			cur = next;
			partition_counts[1]++;
		}
	}

#if HOPCROFT_LOG_PARTITION
	fprintf(stderr, "# htp: partition counts: %lu, %lu\n",
	    partition_counts[0], partition_counts[1]);
#endif

	/* If some were split off, update their state_ecs entries.
	 * Wait to update until after they have all been compared,
	 * otherwise the mix of updated and non-updated states can
	 * lead to inconsistent results. */
	if (partition_counts[0] > 0 && partition_counts[1] > 0) {
		assert(env->ecs[dst_a] != NO_ID);
		assert(env->ecs[dst_b] != NO_ID);

		*ec_dst_a = dst_a;
		*ec_dst_b = dst_b;
		/* env->ec_freelist = freelist_head; */
		res = HTP_PARTITIONED;
	} else if (partition_counts[0] > 0) {
		/* All were split off to ec_dst_a, but should be
		 * restored to the original EC. */
		assert(partition_counts[1] == 0);
		env->ecs[ec_src] = env->ecs[dst_a];

		hopcroft_freelist_push(env, dst_b);
		hopcroft_freelist_push(env, dst_a);
		/* env->ecs[dst_a] = BOX_FREELIST(dst_b); */
		/* env->ecs[dst_b] = freelist_head; */
		/* env->ec_freelist = BOX_FREELIST(dst_a); */
	} else {
		/* All were split off to ec_dst_b, but should be
		 * restored to the original EC. */
		assert(partition_counts[0] == 0);
		assert(partition_counts[1] > 0);
		env->ecs[ec_src] = env->ecs[dst_b];

		/* env->ecs[dst_a] = BOX_FREELIST(dst_b); */
		/* env->ecs[dst_b] = freelist_head; */
		/* env->ec_freelist = BOX_FREELIST(dst_a); */
		hopcroft_freelist_push(env, dst_b);
		hopcroft_freelist_push(env, dst_a);
	}

	return res;
}

static int
hopcroft_schedule_splitter(struct hopcroft_env *env,
    fsm_state_t ec, unsigned char label)
{
	struct splitter *s;
	if (env->stack.count == env->stack.ceil) {
		size_t nceil = 2*env->stack.ceil;
		struct splitter *nsplitters = f_realloc(env->fsm->opt->alloc,
		    env->stack.splitters,
		    nceil * sizeof(env->stack.splitters[0]));
		if (nsplitters == NULL) { return 0; }
		env->stack.ceil = nceil;
		env->stack.splitters = nsplitters;
	}

#if HOPCROFT_LOG_STACK
	fprintf(stderr, "# hopcroft_schedule_splitter: <%d,'%c'>\n",
	    ec, label);
#endif

	s = &env->stack.splitters[env->stack.count];
	s->ec = ec;
	s->label = label;

	/* Update index */
	s->prev = env->stack.index[label];
	env->stack.index[label] = env->stack.count;

	env->stack.count++;
	return 1;
}

static int
hopcroft_update_splitter_stack(struct hopcroft_env *env,
    unsigned char label, fsm_state_t ec_src,
    fsm_state_t ec_dst_a, fsm_state_t ec_dst_b,
    fsm_state_t ec_min)
{
	/* TODO this needs a search index */

	/* If <ec_src, label> is already present then also add
	 * <ec_dst, label> to the stack, otherwise add
	 * <ec_min, label>, which represents whichever
	 * of the new partition sets is smaller .*/
	const size_t s_limit = env->stack.count;

	size_t s_i;
#if 0
	for (s_i = 0; s_i < s_limit; s_i++) {
		struct splitter *s = &env->stack.splitters[s_i];
		if (s->ec == ec_src && s->label == label) {
			/* fprintf(stderr, "XXX remap -> adding <%u,'%c'>\n", ec_dst, label); */
			s->ec = ec_dst_a;
			if (!hopcroft_schedule_splitter(env,
				ec_dst_b, label)) {
				return 0;
			}
			return 1;
		}
	}
#else
	s_i = env->stack.index[label];
	while (s_i < s_limit) {
		struct splitter *s = &env->stack.splitters[s_i];
		assert(s->label == label);
		if (s->label != label) { break; }
		if (s->ec == ec_src) {
			s->ec = ec_dst_a;
			if (!hopcroft_schedule_splitter(env,
				ec_dst_b, label)) {
				return 0;
			}
			return 1;
		}
		s_i = s->prev;
	}
#endif

	/* Not found, add smaller set. */
	return hopcroft_schedule_splitter(env, ec_min, label);
}

static int
hopcroft_grow_updated_ecs(struct hopcroft_env *env)
{
	const size_t nceil = 2*env->updated_ecs_ceil;
	struct updated_ecs *nupdated_ecs = f_realloc(env->fsm->opt->alloc,
	    env->updated_ecs, nceil * sizeof(env->updated_ecs[0]));
	if (nupdated_ecs == NULL) {
		return 0;
	}

	env->updated_ecs_ceil = nceil;
	env->updated_ecs = nupdated_ecs;
	return 1;
}

static int
hopcroft_grow_ecs_freelist(struct hopcroft_env *env)
{
	/* todo: This could double in size up to a certain
	 * point, then add a fixed number of states. */
	const size_t nceil = 2*env->ecs_ceil;
	size_t i;
	fsm_state_t *necs = f_realloc(env->fsm->opt->alloc,
	    env->ecs, nceil * sizeof(env->ecs[0]));
#if HOPCROFT_LOG_FREELIST
	fprintf(stderr, "# growing freelist %lu -> %lu\n", env->ecs_ceil, nceil);
#endif

	if (necs == NULL) {
		return 0;
	}

	for (i = env->ecs_ceil; i < nceil; i++) {
		necs[i] = BOX_FREELIST(i + 1);
	}
	necs[nceil - 1] = NO_ID;
	env->ec_freelist = env->ecs_ceil;

	env->ecs = necs;
	env->ecs_ceil = nceil;
	return 1;
}

/* Build two arrays:
 * - label_index[]: series of states that have an edge with a label,
 *       grouped by label.
 * - label_offset[]: the offset into label_index where the states
 *       who have a particular label start. This array only contains
 *       the actual lables in labels[], which is assumed to be sorted.
 *
 * Note: This function can replace collect_labels above; in that case
 * labels and label_count could be built during the first pass. */
static int
hopcroft_build_label_index(struct hopcroft_env *env,
	const unsigned char *labels, size_t label_count)
{
#if !HOPCROFT_USE_LABEL_INDEX
	(void)env;
	(void)labels;
	(void)label_count;
	return 1;
#else
	const struct fsm *fsm = env->fsm;
	size_t label_counts[256];

	size_t i, edge_count = 0;
	const size_t state_count = fsm->statecount;

	memset(label_counts, 0x00, sizeof(label_counts));

	/* Count the total number of edges, and how many
	 * edges there are with each particular label. */
	for (i = 0; i < state_count; i++) {
		struct fsm_edge e;
		struct edge_iter ei;
		for (edge_set_reset(fsm->states[i].edges, &ei);
		     edge_set_next(&ei, &e); ) {
			edge_count++;
			label_counts[e.symbol]++;
		}
	}

	assert(edge_count > 0);

	/* Histogram the label counts. */
	for (i = 1; i < 256; i++) {
		label_counts[i] += label_counts[i - 1];
	}

	env->label_index = f_malloc(fsm->opt->alloc,
	    edge_count * sizeof(env->label_index[0]));
	if (env->label_index == NULL) {
		return 0;
	}

	env->label_offsets = f_malloc(fsm->opt->alloc,
	    label_count * sizeof(env->label_offsets[0]));
	if (env->label_offsets == NULL) {
		f_free(fsm->opt->alloc, env->label_index);
		return 0;
	}

	/* Save the offsets -- for labels[l_i], the states who have
	 * edges with it will appear between label_offsets[l_i - 1]
	 * and label_offsets[l_i], or 0 when l_i == 0. */
	for (i = 0; i < label_count; i++) {
		env->label_offsets[i] = label_counts[labels[i]];
		if (i > 0) {	/* must be sorted */
			assert(labels[i] > labels[i - 1]);
		}
	}

	/* Use the histogram to group the state IDs by edge label.
	 * label_counts[i] stores the offset where the next instance
	 * of that label's index data should be stored. */
	for (i = state_count; i > 0; i--) {
		struct fsm_edge e;
		struct edge_iter ei;
		for (edge_set_reset(fsm->states[i - 1].edges, &ei);
		     edge_set_next(&ei, &e); ) {
			--label_counts[e.symbol];

#if HOPCROFT_LOG_INDEX
			fprintf(stderr, "label '%c', state[%ld]: count %u\n",
			    e.symbol, i - 1,
			    label_counts[e.symbol]);
#endif
			assert(label_counts[e.symbol] < edge_count);
			env->label_index[label_counts[e.symbol]] = i - 1;
		}
	}

#if HOPCROFT_LOG_INDEX
	for (i = 0; i < label_count; i++) {
		fprintf(stderr, "label_offsets[%lu]: %u\n",
		    i, env->label_offsets[i]);
	}

	for (i = 0; i < edge_count; i++) {
		fprintf(stderr, "label_index[%lu]: %u\n",
		    i, env->label_index[i]);
	}
#endif

#if EXPENSIVE_INTEGRITY_CHECKS
	/* If a state is in the index for a label, it must have that
	 * label, and there must not be any extras. */
	{
		size_t s_i, l_i;
		for (s_i = 0; s_i < state_count; s_i++) {
			struct fsm_edge e;
			struct edge_iter ei;
			unsigned char label;
			size_t has_label_count = 0, found_label_count = 0;
			uint64_t has_label[4] = { 0, 0, 0, 0 };
			for (l_i = 0; l_i < label_count; l_i++) {
				size_t i = (l_i == 0 ? 0 : env->label_offsets[l_i - 1]);
				label = labels[l_i];
				while (i < env->label_offsets[l_i]) {
					if (env->label_index[i] == s_i) {
						uint64_t bit;
						bit = (uint64_t)1 << (label & 63);

						if (!(has_label[label/64] & bit)) {
							has_label[label/64] |= bit;
							has_label_count++;
						}
					}
					i++;
				}
			}

			for (edge_set_reset(fsm->states[s_i].edges, &ei);
			     edge_set_next(&ei, &e); ) {
				label = e.symbol;
				assert(has_label[label/64] & ((uint64_t)1 << (label&63)));
				found_label_count++;
			}

			assert(has_label_count == found_label_count);
		}
	}
#endif

	return 1;
#endif
}

static void
hopcroft_mark_ec_filter(struct hopcroft_env *env,
    unsigned char label)
{
#if HOPCROFT_USE_LABEL_INDEX
	size_t l_i, i;		/* label_i */
	uint64_t *filter = env->ec_filter;
	if (filter == NULL) { return; }

	memset(filter, 0x00, env->ec_filter_bytes);

	for (l_i = 0; l_i < env->label_count; l_i++) {
		size_t base, limit;
		if (env->labels[l_i] != label) { continue; }
		base = (l_i == 0
		    ? 0
		    : env->label_offsets[l_i - 1]);
		limit = env->label_offsets[l_i];
		for (i = base; i < limit; i++) {
			const fsm_state_t s = env->label_index[i];
			const fsm_state_t ec_id = env->state_ecs[s];

			/* Don't mark single-entry ECs. */
			if (env->ecs[ec_id] == s &&
			    env->jump[s] == NO_ID) { continue; }

			filter[ec_id/64] |= ((uint64_t)1 << (ec_id&63));
		}
		break;
	}
#else
	(void)env;
	(void)label;
#endif
}

static int
hopcroft_check_ec_filter(const uint64_t *filter,
    fsm_state_t ec_id)
{
#if HOPCROFT_USE_LABEL_INDEX
	return 0 != (filter[ec_id/64] & ((uint64_t)1 << (ec_id&63)));
#else
	(void)filter;
	(void)ec_id;
	return 1;
#endif
}
#endif

#endif /* MIN_ALT */
