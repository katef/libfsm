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

#define MIN_ALT 1
#define DUMP_MAPPINGS 0
#define DUMP_STEPS 0
#define DUMP_TIME 1

#define NAIVE_DUMP_TABLE 0
#define NAIVE_LOG_PARTITION 0

#define MOORE_DUMP_INIT 0
#define MOORE_DUMP_ECS 0
#define MOORE_LOG_PARTITION 0

#define DO_HOPCROFT 1
#define HOPCROFT_DUMP_INIT 0
#define HOPCROFT_DUMP_ECS 0
#define HOPCROFT_LOG_STACK 0
#define HOPCROFT_LOG_PARTITION 0

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

int
fsm_minimise(struct fsm *fsm)
{
#if MIN_ALT
	int r;
	size_t states_before, states_naive, states_moore, states_hopcroft, states_brz;
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

	if (!fsm_trim(fsm)) {
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

	fprintf(stdout, "# before: %lu, naive: %lu, moore: %lu, hopcroft: %lu, brz: %lu\n",
	    states_before, states_naive, states_moore, states_hopcroft, states_brz);
	fflush(stdout);

	assert(states_naive <= states_brz);
	assert(states_moore == states_naive);

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
#if DUMP_MAPPINGS
		fprintf(stderr, "# naive_mapping[%u]: %u\n", i, mapping[i]);
#endif
	}

	if (minimized_state_count != NULL) {
		*minimized_state_count = new_id;
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
	enum init_ec { INIT_EC_NOT_FINAL, INIT_EC_FINAL, };

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
#if DUMP_MAPPINGS
			fprintf(stderr, "# moore_mapping[%u]: %u\n",
			    cur, mapping[cur]);
#endif
			cur = env.jump[cur];
		}
	}

	if (minimized_state_count != NULL) {
		*minimized_state_count = ec_count - ec_offset;
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

/* This is a hacky first pass at filtering searching for ECs
 * whose edges would actually be split by the splitter.
 * The runtime is currently dominated by scanning edges looking
 * for anything that matches the splitter. */
static void
mark_ec_labels(const struct fsm_state *s, uint64_t *labels)
{
	struct fsm_edge e;
	struct edge_iter ei;
	fsm_state_t id;

	for (edge_set_reset(s->edges, &ei);
	     edge_set_next(&ei, &e); ) {
		unsigned char label = e.symbol;
		labels[label/64] |= ((uint64_t)1 << (label&63));
	}
}

static int
min_hopcroft(const struct fsm *fsm,
    const unsigned char *labels, size_t label_count,
    fsm_state_t *mapping, size_t *minimized_state_count)
{
	/* init splitter stack */
	struct hopcroft_env env;
	int changed, res = 0;
	size_t i, iter = 0, steps = 0;
	fsm_state_t ec_count, ec_offset;

	/* Alloc for each state, plus the dead state. */
	size_t alloc_size = (fsm->statecount + 1) * sizeof(env.state_ecs[0]);

	/* Note that the ordering here is significant -- the
	 * not-final EC may be skipped below. */
	enum init_ec { INIT_EC_NOT_FINAL, INIT_EC_FINAL, };

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
	env.stack.splitters = NULL;
	ec_count = 0;

	assert(fsm->statecount != 0);

	env.stack.splitters = f_malloc(fsm->opt->alloc,
	    DEF_SPLITTER_STACK_CEIL * sizeof(env.stack.splitters[0]));
	if (env.stack.splitters == NULL) { goto cleanup; }
	env.stack.ceil = DEF_SPLITTER_STACK_CEIL;
	env.stack.count = 0;

	env.state_ecs = f_malloc(fsm->opt->alloc, alloc_size);
	if (env.state_ecs == NULL) { goto cleanup; }

	env.jump = f_malloc(fsm->opt->alloc, alloc_size);
	if (env.jump == NULL) { goto cleanup; }

	env.ecs = f_malloc(fsm->opt->alloc, alloc_size);
	if (env.ecs == NULL) { goto cleanup; }

	env.ec_labels = f_calloc(fsm->opt->alloc,
	    4*(fsm->statecount + 1), sizeof(env.ec_labels[0]));
	if (env.ec_labels == NULL) { goto cleanup; }

	env.ecs[INIT_EC_NOT_FINAL] = NO_ID;
	env.ecs[INIT_EC_FINAL] = NO_ID;
	ec_count = 2;

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

			mark_ec_labels(&fsm->states[i], &env.ec_labels[4*ec]);
		}

		/* The dead state is an honorary member of the not-final EC.
		 * Only add it if necessary. */
		if (!fsm_all(fsm, fsm_iscomplete)) {
			env.state_ecs[env.dead_state] = INIT_EC_NOT_FINAL;
			env.jump[env.dead_state] = env.ecs[INIT_EC_NOT_FINAL];
			env.ecs[INIT_EC_NOT_FINAL] = env.dead_state;
			ecs_counts[INIT_EC_NOT_FINAL]++;
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
	for (i = 0; i < ec_count; i++) {
		fprintf(stderr, "# --ec[%lu]: %d\n", i, env.ecs[i]);
	}

	dump_edges(env.fsm);
#endif

	while (env.stack.count > 0) { /* until empty */
		size_t l_i, ec_i, cur_ec_count;
		struct splitter *splitter;
		fsm_state_t splitter_ec;
		unsigned char splitter_label;

#if HOPCROFT_DUMP_ECS
		i = 0;
		fprintf(stderr, "# iter %ld, steps %lu\n", iter, steps);
		while (i < ec_count) {
			fprintf(stderr, "H_EC[%lu]:", i);
			moore_dump_ec(stderr, env.ecs[i], env.jump);
			fprintf(stderr, "\n");
			i++;
		}
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
#if HOPCROFT_LOG_STACK
		fprintf(stderr, "# popped splitter: <%d,'%c'>\n",
		    splitter_ec, splitter_label);
#endif

		/* This loop can add new ECs, but they should not
		 * be counted until the next pass. */
		cur_ec_count = ec_count;

		for (ec_i = 0; ec_i < cur_ec_count; ec_i++) {
			const fsm_state_t ec_src = ec_i;
			const fsm_state_t ec_dst = ec_count;
			size_t l_i;
			size_t pcounts[2]; /* partition counts */

			/* Don't bother checking an EC if it doesn't contain the label. */
			if (!(env.ec_labels[4*ec_src + splitter_label/64] & ((uint64_t)1 << (splitter_label & 63)))) {
				continue;
			}

			steps++;
			if (hopcroft_try_partition(&env,
				splitter_ec, splitter_label,
				ec_src, ec_dst, pcounts)) {
				const fsm_state_t ec_min = pcounts[0] < pcounts[1]
				    ? ec_src : ec_dst;
				ec_count++;
#if HOPCROFT_LOG_PARTITION
				fprintf(stderr, "# partition %lu, %lu -> ec_min %u\n",
				    pcounts[0], pcounts[1], ec_min);
#endif

				for (l_i = 0; l_i < label_count; l_i++) {
					const unsigned char label = labels[l_i];
					if (!hopcroft_update_splitter_stack(&env,
						label, ec_src, ec_dst, ec_min)) {
						goto cleanup;
					}
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
			/* Don't include the dead state in the result. */
			if (cur == env.dead_state) {
				ec_offset++;
				cur = env.jump[cur];
				if (cur == NO_ID) { break; }
			}

			mapping[cur] = i - ec_offset;
#if DUMP_MAPPINGS
			fprintf(stderr, "# hopcroft_mapping[%u]: %u\n",
			    cur, mapping[cur]);
#endif
			cur = env.jump[cur];
		}
	}

	if (minimized_state_count != NULL) {
		*minimized_state_count = ec_count - ec_offset;
	}

#if DUMP_STEPS
	fprintf(stderr, "# done in %lu iteration(s), %lu step(s), %ld -> %lu states, label_count %lu\n",
            iter, steps, fsm->statecount, *minimized_state_count, label_count);
#endif

	res = 1;
	/* fall through */
cleanup:
	f_free(fsm->opt->alloc, env.ecs);
	f_free(fsm->opt->alloc, env.ec_labels);
	f_free(fsm->opt->alloc, env.state_ecs);
	f_free(fsm->opt->alloc, env.jump);
	f_free(fsm->opt->alloc, env.stack.splitters);
	return res;
}

static void
hopcroft_dump_ec(FILE *f, fsm_state_t start, const fsm_state_t *jump)
{
	return moore_dump_ec(f, start, jump);
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
		}
		/* could skip rest if ordered */
	}

	return fsm->statecount;	/* dead end */
}

static int
hopcroft_try_partition(struct hopcroft_env *env,
    fsm_state_t ec_splitter, unsigned char label,
    fsm_state_t ec_src, fsm_state_t ec_dst,
    size_t partition_counts[2])
{
	int res = 0;
	uint64_t *ec_labels;

	/* Partition EC[ec_src] into the sets that have edges
	 * with label that lead to EC_SPLITTER and those that
	 * don't. */
	fsm_state_t cur = env->ecs[ec_src];
	fsm_state_t to, to_ec, prev = NO_ID;

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

#if HOPCROFT_LOG_PARTITION
	fprintf(stderr, "# --- htp: checking '%c' for %u\n", label, ec_src);
#endif

	/* initialize the new EC -- empty */
	env->ecs[ec_dst] = NO_ID;
	ec_labels = &env->ec_labels[4 * ec_dst];

	partition_counts[0] = 0;
	partition_counts[1] = 0;

	while (cur != NO_ID) {
		to = hopcroft_delta(env->fsm, cur, label);
		to_ec = env->state_ecs[to];

#if HOPCROFT_LOG_PARTITION
		fprintf(stderr, "# --- htp: next, cur %u, has to %d -> to_ec %d\n", cur, to, to_ec);
#endif

		/* fixme: should the split-off EC be for ones that
		 * do or don't match ec_splitter? */
		if (to_ec != ec_splitter) {
#if HOPCROFT_LOG_PARTITION
			fprintf(stderr, "# htp: keeping -- prev %d, cur %u, next %d\n",
			    prev, cur, env->jump[cur]);
#endif
			prev = cur;
			cur = env->jump[cur];
			partition_counts[0]++;
		} else {	/* unlink, split */
			/* Unlink this state from the current EC,
			 * instead put it as the start of a new one
			 * at ecs[ec_dst]. */
			fsm_state_t next = env->jump[cur];

#if HOPCROFT_LOG_PARTITION
			fprintf(stderr, "# htp: unlinking -- label '%c', src %u, dst %u, ec_splitter %d, cur %u -> to_ec %d\n", label, ec_src, ec_dst, ec_splitter, cur, to_ec);
			fprintf(stderr, "#                -- prev %d, cur %u, next %d\n",
			    prev, cur, env->jump[cur]);
#endif

			if (prev != NO_ID) {
				env->jump[prev] = next;
			} else {
				env->ecs[ec_src] = next;
			}

			env->jump[cur] = env->ecs[ec_dst];
			env->ecs[ec_dst] = cur;
			cur = next;
			partition_counts[1]++;
		}
	}

	/* If some were split off, update their state_ecs entries.
	 * Wait to update until after they have all been compared,
	 * otherwise the mix of updated and non-updated states can
	 * lead to inconsistent results. */
	if (partition_counts[0] > 0 && partition_counts[1] > 0) {
		assert(env->ecs[ec_dst] != NO_ID);
		cur = env->ecs[ec_dst];

		memset(ec_labels, 0x00, 4 * sizeof(uint64_t));

		while (cur != NO_ID) {
			mark_ec_labels(&env->fsm->states[cur], ec_labels);
			env->state_ecs[cur] = ec_dst;
			cur = env->jump[cur];
		}
		res = 1;
	} else if (partition_counts[1] > 0) {
		/* All were split off to ec_dst, but should be restored
		 * to the original EC. */
		assert(partition_counts[0] == 0);
		env->ecs[ec_src] = env->ecs[ec_dst];
		env->ecs[ec_dst] = NO_ID;
	} else {
		assert(partition_counts[1] == 0);
		/* all in the original, no-op */
	}

#if EXPENSIVE_INTEGRITY_CHECKS
	psrc_count = 0;
	cur = env->ecs[ec_src];
	/* fprintf(stderr, "# H_src:"); */
	while (cur != NO_ID) {
		/* fprintf(stderr, " %d", cur); */
		cur = env->jump[cur];
		psrc_count++;
	}
	/* fprintf(stderr, "\n# H_dst:"); */

	pdst_count = 0;
	cur = env->ecs[ec_dst];
	while (cur != NO_ID) {
		/* fprintf(stderr, " %d", cur); */
		cur = env->jump[cur];
		pdst_count++;
	}
	/* fprintf(stderr, "\n"); */

	assert(state_count == psrc_count + pdst_count);
#endif

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
	env->stack.count++;
	return 1;
}

static int
hopcroft_update_splitter_stack(struct hopcroft_env *env,
    unsigned char label, fsm_state_t ec_src, fsm_state_t ec_dst,
    fsm_state_t ec_min)
{
	/* If <ec_src, label> is already present then also add
	 * <ec_dst, label> to the stack, otherwise add
	 * <ec_min, label>, which represents whichever
	 * of the new partition sets is smaller .*/
	const size_t s_limit = env->stack.count;
	size_t s_i;
	for (s_i = 0; s_i < s_limit; s_i++) {
		struct splitter *s = &env->stack.splitters[s_i];
		if (s->ec == ec_src && s->label == label) {
			if (!hopcroft_schedule_splitter(env,
				ec_dst, label)) {
				return 0;
			}
			return 1;
		}
	}

	/* Not found, add smaller set. */
	return hopcroft_schedule_splitter(env, ec_min, label);
}

#endif /* MIN_ALT */
