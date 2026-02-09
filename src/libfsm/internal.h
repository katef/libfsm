/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_INTERNAL_H
#define FSM_INTERNAL_H

#include <limits.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include <adt/common.h>

struct bm;
struct edge_set;
struct state_set;
struct state_array;
struct linkage_info;

/*
 * The alphabet (Sigma) for libfsm's FSM is arbitrary octets.
 * These octets may or may not spell out UTF-8 sequences,
 * depending on the context in which the FSM is used.
 *
 * Octets are stored as unsigned char for orderability
 * independent of the signedness of char.
 */

/*
 * The highest value of an symbol, the maximum value in Sigma.
 */
#define FSM_SIGMA_MAX UCHAR_MAX

/*
 * The number of non-special symbols in the alphabet.
 * This is the number of symbols with the value <= UCHAR_MAX.
 */
#define FSM_SIGMA_COUNT (FSM_SIGMA_MAX + 1)

#define FSM_ENDCOUNT_MAX ULONG_MAX

#define FSM_CAPTURE_MAX INT_MAX

struct fsm_edge {
	fsm_state_t state:24; /* destination. :24 for packing */
	unsigned char symbol;
};

struct fsm_state {
	struct edge_set *edges;
	struct state_set *epsilons;

	unsigned int end:1;

	/* If 0, then this state has no need for checking
	 * the fsm->capture_info struct. */
	unsigned int has_capture_actions:1;

	/* meaningful within one particular transformation only */
	unsigned int visited:1;

	/* If 0, then this state has no need for checking
	 * the fsm->eager_output_info struct. */
	unsigned int has_eager_outputs:1;
};

struct fsm {
	struct fsm_state *states; /* array */
	const struct fsm_alloc *alloc;

	size_t statealloc; /* number of elements allocated */
	size_t statecount; /* number of elements populated */
	size_t endcount:31; /* :31 for packing */

	unsigned int hasstart:1;
	fsm_state_t start;

	struct fsm_capture_info *capture_info;
	struct endid_info *endid_info;
	struct eager_output_info *eager_output_info;
	struct linkage_info *linkage_info;
};

#define LINKAGE_NO_STATE ((fsm_state_t)-1)

/* Internal structure for storing structural info about an NFA.
 * This is currently only used by fsm_union_repeated_pattern_group,
 * which needs to identify a couple components of the NFA in order
 * to link groups of repeated pattern together correctly. */
struct linkage_info {
	/* The states with a /./ self edge representing the unanchored
	 * start and end, or LINKAGE_NO_STATE. There can be at most one
	 * of each. */
	fsm_state_t unanchored_start_loop;
	fsm_state_t unanchored_end_loop;

	/* The end state following the unanchored end loop. */
	fsm_state_t unanchored_end_loop_end;

	/* States that link to paths only reachable from the beginning of input. */
	struct state_set *anchored_starts;
	/* States leading to an anchored end. */
	struct state_set *anchored_ends;
};

struct fsm *
fsm_mergeab(struct fsm *a, struct fsm *b,
    fsm_state_t *base_b);

int
state_hasnondeterminism(const struct fsm *fsm, fsm_state_t state, struct bm *bm);

/*
 * TODO: if this were a public API, we could present ragged array of { a, n } structs
 * for states, with wrapper to populate malloced array of user-facing structs.
 */
struct state_set **
epsilon_closure(const struct fsm *fsm);

void
closure_free(const struct fsm *fsm, struct state_set **closures, size_t n);

/*
 * Internal free function that invokes free(3) by default, or a user-provided
 * free function to free memory and perform any custom memory tracking or handling
 */
void
f_free(const struct fsm_alloc *a, void *p);

/*
 * Internal malloc function that invokes malloc(3) by default, or a user-provided
 * malloc function to allocate memory and perform any custom memory tracking or handling
 */
void *
f_malloc(const struct fsm_alloc *a, size_t sz);

/*
 * Internal calloc function that invokes calloc(3) by default, or a user-provided
 * calloc function to allocate memory and perform any custom memory tracking or handling,
 * including initializing the memory to zero.
 */
void *
f_calloc(const struct fsm_alloc *a, size_t n, size_t sz);

/*
 * Internal realloc function that invokes realloc(3) by default, or a user-provided
 * realloc function to re-allocate memory to the specified size and perform
 * any custom memory tracking or handling
 */
void *
f_realloc(const struct fsm_alloc *a, void *p, size_t sz);

/* Take a source fsm and a state mapping, produce a new
 * fsm where states may be consolidated. */
struct fsm *
fsm_consolidate(const struct fsm *src,
    const fsm_state_t *mapping, size_t mapping_count);

#endif
