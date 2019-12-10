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
#include <fsm/options.h>

struct edge_set;
struct state_set;
struct state_array;

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

struct fsm_edge {
	struct state_set *sl;
	unsigned char symbol;
};

struct fsm_state {
	unsigned int end:1;

	/* meaningful within one particular transformation only */
	unsigned int visited:1;

	struct edge_set *edges;
	struct state_set *epsilons;

	void *opaque;
};

struct fsm {
	struct fsm_state *states; /* array */

	size_t statealloc; /* number of elements allocated */
	size_t statecount; /* number of elements populated */
	size_t endcount;

	fsm_state_t start;
	unsigned int hasstart:1;

	const struct fsm_options *opt;
};

struct fsm_edge *
fsm_hasedge_literal(const struct fsm *fsm, fsm_state_t state, char c);

int
fsm_addedge_bulk(struct fsm *fsm,
	fsm_state_t from, fsm_state_t *a, size_t n,
	char c);

void
fsm_carryopaque_array(struct fsm *src_fsm, const fsm_state_t *src_set, size_t n,
    struct fsm *dst_fsm, fsm_state_t dst_state);

void
fsm_carryopaque(struct fsm *fsm, const struct state_set *set,
	struct fsm *new, fsm_state_t state);

struct fsm *
fsm_mergeab(struct fsm *a, struct fsm *b,
    fsm_state_t *base_b);

struct state_set *
epsilon_closure(const struct fsm *fsm, fsm_state_t state, struct state_set **closure);

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
 * Internal realloc function that invokes realloc(3) by default, or a user-provided
 * realloc function to re-allocate memory to the specified size and perform
 * any custom memory tracking or handling
 */
void *
f_realloc(const struct fsm_alloc *a, void *p, size_t sz);

#endif

