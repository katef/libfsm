/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/out.h>
#include <fsm/options.h>
#include <fsm/bool.h>

#include "internal.h"

enum { PAIR_POOL_SIZE = 1024 };

struct fsm *
fsm_intersect(struct fsm *a, struct fsm *b)
{
	struct fsm *q;

	assert(a != NULL);
	assert(b != NULL);

	if (a->opt != b->opt) {
		errno = EINVAL;
		return NULL;
	}

	/*
	 * This is intersection implemented in terms of complements and union,
	 * by DeMorgan's theorem:
	 *
	 *     a \n b = ~(~a \u ~b)
	 *
	 * This could also be done by walking sets of states through both FSM
	 * simultaneously, as described by Hopcroft, Motwani and Ullman
	 * (2001, 2nd ed.) 4.2, Closure under Intersection. However this way
	 * is simpler to implement.
	 */

	if (!fsm_complement(a)) {
		return NULL;
	}

	if (!fsm_complement(b)) {
		return NULL;
	}

	q = fsm_union(a, b);
	if (q == NULL) {
		return NULL;
	}

	if (!fsm_complement(q)) {
		goto error;
	}

	return q;

error:

	fsm_free(q);

	return NULL;
}

struct pair {
	struct fsm_state *a;
	struct fsm_state *b;
	struct fsm_state *comb;
};

static int cmp_pair(const void *a, const void *b)
{
	const struct pair *pa = a, *pb = b;
	ptrdiff_t delta;
	delta = pa->a - pb->a;
	if (delta < 0) {
		return -1;
	}
	
	if (delta > 0) {
		return 1;
	}

	delta = pa->b - pb->b;
	if (delta < 0) {
		return -1;
	}

	if (delta > 0) {
		return 1;
	}

	return 0;
}

struct pair_pool {
	struct pair_pool *next;
	struct pair items[PAIR_POOL_SIZE];
};

struct bywalk_arena {
	struct pair_pool *head;
	size_t top;

	struct fsm *new;
	struct set *states;
};

static void
free_bywalk(struct bywalk_arena *A)
{
	struct pair_pool *p, *next;

	if (A->states) {
		set_free(A->states);
	}

	if (A->new) {
		fsm_free(A->new);
	}

	for (p = A->head; p != NULL; p = next) {
		next = p->next;
		free(p);
	}
}

static struct pair *alloc_pair(struct bywalk_arena *A)
{
	static const struct pair zero;

	struct pair *item;
	struct pair_pool *pool;

	if (A->head == NULL) {
		goto new_pool;
	}

	if (A->top >= (sizeof A->head->items / sizeof A->head->items[0])) {
		goto new_pool;
	}

	item = &A->head->items[A->top++];
	*item = zero;
	return item;

new_pool:
	pool = malloc(sizeof *pool);
	if (pool == NULL) {
		return NULL;
	}

	pool->next = A->head;
	A->head = pool;
	A->top = 1;

	item = &pool->items[0];
	*item = zero;
	return item;
}

static struct pair *new_pair(struct bywalk_arena *A, struct fsm_state *a, struct fsm_state *b)
{
	struct pair lkup, *p;
	struct fsm_state *combined;

	lkup.a = a;
	lkup.b = b;

	assert(A->states);

	p = set_contains(A->states, &lkup);
	if (p != NULL) {
		return p;
	}

	p = alloc_pair(A);
	if (p == NULL) {
		return NULL;
	}

	combined = fsm_addstate(A->new);
	if (combined == NULL) {
		return NULL;
	}
	combined->equiv = NULL;

	p->a = a;
	p->b = b;
	p->comb = combined;
	if (!set_add(&A->states, p)) {
		return NULL;
	}

	if (a->end && b->end) {
		const struct fsm_options *opt;

		fsm_setend(A->new, combined, 1);

		opt = A->new->opt;
		if (opt != NULL && opt->carryopaque != NULL) {
			const struct fsm_state *states[2];
			states[0] = a;
			states[1] = b;
			/* this is slightly cheesed, but it avoids
			 * constructing a set just to pass these two
			 * states to the carryopaque function
			 */
			opt->carryopaque(states, 2, A->new, combined);
		}
	}

	return p;
}

static int
intersection_walk_edges(struct bywalk_arena *A, struct fsm *a, struct fsm *b, struct pair *start);

static void
mark_equiv_null(struct fsm *fsm);

struct fsm *
fsm_intersect_bywalk(struct fsm *a, struct fsm *b)
{
	/* intersection via DFS of the two DFAs.
	 *
	 * We do this in a few steps:
	 *
	 *   1. Initialize things:
	 *        a) Initialize state map, which maps (q0,q1) pairs to
	 *           new states.
	 *        b) Let start0,start1 be the starting states of the two
	 *           DFAs.  Allocate a new combined state for
	 *           (start0,start1) and add the combined state to the
	 *           queue.
	 *
	 *        c) We use the internal state equiv field as a
	 *           'visited' marker.  If st->equiv == NULL, the
	 *           state has not yet been visited.
	 */

	static const struct bywalk_arena zero;
	struct bywalk_arena A = zero;

	struct fsm *new = NULL;
	struct fsm_state *sa, *sb;
	struct pair *pair0;

	assert(a != NULL);
	assert(b != NULL);

	A.new = fsm_new(a->opt);
	if (A.new == NULL) {
		goto error;
	}

	A.states = set_create(cmp_pair);
	if (A.states == NULL) {
		goto error;
	}

	sa = fsm_getstart(a);
	sb = fsm_getstart(b);

	if (sa == NULL || sb == NULL) {
		/* intersection will be empty.  XXX - should this be an error? */
		goto finish;
	}

	pair0 = new_pair(&A, sa,sb);
	if (pair0 == NULL) {
		goto error;
	}

	assert(pair0->a == sa);
	assert(pair0->b == sb);
	assert(pair0->comb != NULL);
        assert(pair0->comb->equiv == NULL); /* comb not yet been traversed */

	fsm_setstart(A.new, pair0->comb);
	if (!intersection_walk_edges(&A, a,b, pair0)) {
		goto error;
	}

finish:
	new = A.new;
	A.new = NULL; /* avoid freeing new FSM */

	/* reset all equiv fields in the states */
	mark_equiv_null(new);

	free_bywalk(&A);
	return new;

error:
	free_bywalk(&A);
	return NULL;
}

static void
mark_equiv_null(struct fsm *fsm)
{
	struct fsm_state *src;

	assert(fsm != NULL);

	for (src = fsm->sl; src != NULL; src = src->next) {
		src->equiv = NULL;
	}
}

static int
intersection_walk_edges(struct bywalk_arena *A, struct fsm *a, struct fsm *b, struct pair *start)
{
	struct fsm_state *qa, *qb, *qc;
	struct set_iter ei;
	const struct fsm_edge *ea, *eb;

	assert(a != NULL);
	assert(b != NULL);

	assert(A->new != NULL);
	assert(A->states != NULL);

	assert(start != NULL);

	/* This performs the actual intersection by a depth-first search. */
	qa = start->a;
	qb = start->b;
	qc = start->comb;

	assert(qa != NULL);
	assert(qb != NULL);
	assert(qc != NULL);

	if (qc->equiv != NULL) {
		/* already visited combined state */
		return 1;
	}

	/* mark combined state as visited */
	qc->equiv = qc;

	for (ea = set_first(qa->edges, &ei); ea != NULL; ea=set_next(&ei)) {
		struct set_iter dia, dib;
		const struct fsm_state *da, *db;

                /* For each A in alphabet:
                 *   if an edge exists with label A in both FSMs, follow it
                 */
		eb = fsm_hasedge(qb, ea->symbol);
		if (eb == NULL) {
			continue;
		}

		for (da = set_first(ea->sl, &dia); da != NULL; da=set_next(&dia)) {
			for (db = set_first(eb->sl, &dib); db != NULL; db = set_next(&dib)) {
				struct pair *dst;

				/* FIXME: deal with annoying const-ness here */
				dst = new_pair(A, (struct fsm_state *)da, (struct fsm_state *)db);

				if (!fsm_addedge(qc, dst->comb, ea->symbol)) {
					return 0;
				}

                                /* depth-first traversal of the graphs */
				if (!intersection_walk_edges(A, a,b, dst)) {
					return 0;
				}
			}
		}
	}

	return 1;
}


