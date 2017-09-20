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

/* XXX - revisit what would be a good size for this. */
enum { STATE_TUPLE_POOL_SIZE = 1024 };

/* Tuple (a,b,comb) for intersection.  a & b are the states of the original
 * FSMs.  comb is the state of the combined FSM.
 */
struct state_tuple {
	struct fsm_state *a;
	struct fsm_state *b;
	struct fsm_state *comb;
};

/* comparison of state_tuples for the (ordered) set */
static int cmp_state_tuple(const void *a, const void *b)
{
	const struct state_tuple *pa = a, *pb = b;
	ptrdiff_t delta;

        /* XXX - do we need to specially handle NULLs? */

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

struct state_tuple_pool {
	struct state_tuple_pool *next;
	struct state_tuple items[STATE_TUPLE_POOL_SIZE];
};

enum {
	ENDCHECK_NEITHER = 1 << 0x0,
	ENDCHECK_ONLYB   = 1 << 0x1,
	ENDCHECK_ONLYA   = 1 << 0x2,
	ENDCHECK_BOTH    = 1 << 0x3
};

enum {
	ENDCHECK_UNION = ENDCHECK_ONLYA | ENDCHECK_ONLYB | ENDCHECK_BOTH,
	ENDCHECK_INTERSECT = ENDCHECK_BOTH,
	ENDCHECK_SUBTRACT  = ENDCHECK_ONLYA
};

struct bywalk_arena {
	struct state_tuple_pool *head;
	size_t top;

	struct fsm *new;
	struct set *states;

	/* table for which combinations are valid bits.
	 * There are four combinations:
	 *
	 *   a_end  b_end    AB		bit	endcheck
	 *   false  false    00		0	0x1
	 *   false  true     01		1	0x2
	 *   true   false    10		2	0x4
	 *   true   true     11		3	0x8
	 *
	 * Here bit is the bit that expresses whether that combination
	 * is valid or not.  We need four bits.
	 *
	 * Operation	Requirement			endcheck
	 * intersect    both true			0x8
	 * subtract     first true, second false	0x4
	 * union	either true			0xE
	 */
	unsigned int endcheck:4;
};

static void
free_bywalk(struct bywalk_arena *ar)
{
	struct state_tuple_pool *p, *next;

	if (ar->states) {
		set_free(ar->states);
	}

	if (ar->new) {
		fsm_free(ar->new);
	}

	for (p = ar->head; p != NULL; p = next) {
		next = p->next;
		free(p);
	}
}

static struct state_tuple *alloc_state_tuple(struct bywalk_arena *ar)
{
	static const struct state_tuple zero;

	struct state_tuple *item;
	struct state_tuple_pool *pool;

	if (ar->head == NULL) {
		goto new_pool;
	}

	if (ar->top >= (sizeof ar->head->items / sizeof ar->head->items[0])) {
		goto new_pool;
	}

	item = &ar->head->items[ar->top++];
	*item = zero;
	return item;

new_pool:
	pool = malloc(sizeof *pool);
	if (pool == NULL) {
		return NULL;
	}

	pool->next = ar->head;
	ar->head = pool;
	ar->top = 1;

	item = &pool->items[0];
	*item = zero;
	return item;
}

static struct state_tuple *new_state_tuple(struct bywalk_arena *ar, struct fsm_state *a, struct fsm_state *b)
{
	struct state_tuple lkup, *p;
	struct fsm_state *comb;
	const struct fsm_options *opt;
	int endbit, is_end; 

	lkup.a = a;
	lkup.b = b;

	assert(ar->states);

	p = set_contains(ar->states, &lkup);
	if (p != NULL) {
		return p;
	}

	p = alloc_state_tuple(ar);
	if (p == NULL) {
		return NULL;
	}

	comb = fsm_addstate(ar->new);
	if (comb == NULL) {
		return NULL;
	}
	comb->equiv = NULL;

	p->a = a;
	p->b = b;
	p->comb = comb;
	if (!set_add(&ar->states, p)) {
		return NULL;
	}

	endbit = ((a && a->end) << 1) | (b && b->end);
	is_end = ar->endcheck & (1 << endbit);

	fprintf(stderr, "endbit = %d.  is_end = %d.  endcheck = %d.  a = %p, b = %p, a->end = %d, b->end = %d\n",
		endbit, is_end, ar->endcheck, (void *)a, (void *)b, a ? (int)a->end : 0, b ? (int)b->end : 0);

	if (is_end) {
		fsm_setend(ar->new, comb, 1);

		opt = ar->new->opt;
		if (opt != NULL && opt->carryopaque != NULL) {
			const struct fsm_state *states[2];
			states[0] = a;
			states[1] = b;
			/* this is slightly cheesed, but it avoids
			 * constructing a set just to pass these two
			 * states to the carryopaque function
			 */
			opt->carryopaque(states, 2, ar->new, comb);
		}
	}

	return p;
}

static int
intersection_walk_edges(struct bywalk_arena *ar, struct fsm *a, struct fsm *b, struct state_tuple *start);

static void
mark_equiv_null(struct fsm *fsm);

struct fsm *
fsm_intersect(struct fsm *a, struct fsm *b)
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
	struct bywalk_arena ar = zero;

	struct fsm *new = NULL;
	struct fsm_state *sa, *sb;
	struct state_tuple *tup0;

	assert(a != NULL);
	assert(b != NULL);

	ar.new = fsm_new(a->opt);
	if (ar.new == NULL) {
		goto error;
	}

	ar.states = set_create(cmp_state_tuple);
	if (ar.states == NULL) {
		goto error;
	}

	ar.endcheck = ENDCHECK_INTERSECT;

	sa = fsm_getstart(a);
	sb = fsm_getstart(b);

	if (sa == NULL || sb == NULL) {
                /* if one of the FSMs lacks a start state, the
                 * intersection will be empty */
		goto finish;
	}

	tup0 = new_state_tuple(&ar, sa,sb);
	if (tup0 == NULL) {
		goto error;
	}

	assert(tup0->a == sa);
	assert(tup0->b == sb);
	assert(tup0->comb != NULL);
        assert(tup0->comb->equiv == NULL); /* comb not yet been traversed */

	fsm_setstart(ar.new, tup0->comb);
	if (!intersection_walk_edges(&ar, a,b, tup0)) {
		goto error;
	}

finish:
	new = ar.new;
	ar.new = NULL; /* avoid freeing new FSM */

	/* reset all equiv fields in the states */
	mark_equiv_null(new);

	free_bywalk(&ar);
	return new;

error:
	free_bywalk(&ar);
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
intersection_walk_edges(struct bywalk_arena *ar, struct fsm *a, struct fsm *b, struct state_tuple *start)
{
	struct fsm_state *qa, *qb, *qc;
	struct set_iter ei;
	const struct fsm_edge *ea, *eb;

	assert(a != NULL);
	assert(b != NULL);

	assert(ar->new != NULL);
	assert(ar->states != NULL);

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
				struct state_tuple *dst;

				/* FIXME: deal with annoying const-ness here */
				dst = new_state_tuple(ar, (struct fsm_state *)da, (struct fsm_state *)db);

				if (!fsm_addedge(qc, dst->comb, ea->symbol)) {
					return 0;
				}

                                /* depth-first traversal of the graphs */
				if (!intersection_walk_edges(ar, a,b, dst)) {
					return 0;
				}
			}
		}
	}

	return 1;
}

static int
subtract_walk_edges(struct bywalk_arena *ar, struct fsm *a, struct fsm *b, struct state_tuple *start);

struct fsm *
fsm_subtract_bywalk(struct fsm *a, struct fsm *b)
{
	/* subtract via DFS of the two DFAs.
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
	struct bywalk_arena ar = zero;

	struct fsm *new = NULL;
	struct fsm_state *sa, *sb;
	struct state_tuple *tup0;

	assert(a != NULL);
	assert(b != NULL);

	sa = fsm_getstart(a);
	sb = fsm_getstart(b);

	if (sa == NULL) {
		/* if A lacks a start state, the
		 * subtraction will be empty */
		goto finish;
	}

	if (sb == NULL) {
		/* if B lacks a start state, the 
		 * subtraction will be equal to A
		 */
		return fsm_clone(a);
	}

	ar.new = fsm_new(a->opt);
	if (ar.new == NULL) {
		goto error;
	}

	ar.states = set_create(cmp_state_tuple);
	if (ar.states == NULL) {
		goto error;
	}

	ar.endcheck = ENDCHECK_SUBTRACT;

	tup0 = new_state_tuple(&ar, sa,sb);
	if (tup0 == NULL) {
		goto error;
	}

	assert(tup0->a == sa);
	assert(tup0->b == sb);
	assert(tup0->comb != NULL);
        assert(tup0->comb->equiv == NULL); /* comb not yet been traversed */

	fsm_setstart(ar.new, tup0->comb);
	if (!subtract_walk_edges(&ar, a,b, tup0)) {
		goto error;
	}

	/* subtraction can have unreachable states.  trim those away. */
	/*
	if (!fsm_trim(ar.new)) {
		goto error;
	}
	*/

finish:
	new = ar.new;
	ar.new = NULL; /* avoid freeing new FSM */

	/* reset all equiv fields in the states */
	mark_equiv_null(new);

	free_bywalk(&ar);
	return new;

error:
	free_bywalk(&ar);
	return NULL;
}

static int
subtract_walk_edges(struct bywalk_arena *ar, struct fsm *a, struct fsm *b, struct state_tuple *start)
{
	struct fsm_state *qa, *qb, *qc;
	struct set_iter ei;
	const struct fsm_edge *ea, *eb;

	assert(a != NULL);
	assert(b != NULL);

	assert(ar->new != NULL);
	assert(ar->states != NULL);

	assert(start != NULL);

	/* This performs the actual subtraction by a depth-first search. */
	qa = start->a;
	qb = start->b;
	qc = start->comb;

	assert(qa != NULL);
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
		eb = qb ? fsm_hasedge(qb, ea->symbol) : NULL;
		for (da = set_first(ea->sl, &dia); da != NULL; da=set_next(&dia)) {
			db = eb ? set_first(eb->sl, &dib) : NULL;

			for (;;) {
				struct state_tuple *dst;

				/* FIXME: deal with annoying const-ness here */
				dst = new_state_tuple(ar, (struct fsm_state *)da, (struct fsm_state *)db);

				if (!fsm_addedge(qc, dst->comb, ea->symbol)) {
					return 0;
				}

                                /* depth-first traversal of the graphs */
				if (!subtract_walk_edges(ar, a,b, dst)) {
					return 0;
				}


				if (db != NULL) {
					db = set_next(&dib);
				}

				if (db == NULL) {
					break;
				}
			}
		}
	}

	return 1;
}
