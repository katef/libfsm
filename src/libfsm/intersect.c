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
#include <fsm/pred.h>
#include <fsm/walk.h>

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

	delta = (pa->a > pb->a) - (pa->a < pb->a);
	if (delta == 0) {
		delta = (pa->b > pb->b) - (pa->b < pb->b);
	}

	return delta;
}

struct state_tuple_pool {
	struct state_tuple_pool *next;
	struct state_tuple items[STATE_TUPLE_POOL_SIZE];
};

/* Bit values for walking two fsms together.  Used to constrain what
 * edges will be followed and what combined states can be considered end
 * states.
 *
 * There are four bits because there are four possible states (NEITHER,
 * ONLYA, ONLYB, BOTH).  We use these bits as a fast table lookup.
 *
 * When walking two graphs, the caller orders them (A,B).  This is
 * important for some non-commutative binary operations, like
 * subtraction, where A-B != B-A.
 *
 * The names below assume that the order is (A,B), but make no
 * assumptions about what that means.
 *
 * These values can be bitwise OR'd together.  So, to find the states
 * for subtraction A-B, the mask would be:
 *      FSM_WALK2_ONLYA | FSM_WALK2_BOTH
 */
enum {
	FSM_WALK2_NEITHER = 1 << 0x0,   /* To complete the four possible states */
	FSM_WALK2_ONLYB   = 1 << 0x1,   /* Follow edges that B has and A does not */
	FSM_WALK2_ONLYA   = 1 << 0x2,   /* Follow edges that A has and B does not */
	FSM_WALK2_BOTH    = 1 << 0x3    /* Follow edges that both A and B have */
};

/* Constraints for walking edges for union (A|B), intersection (A&B), and
 * subtraction (A-B)
 */
enum {
	FSM_WALK2_EDGE_UNION     = FSM_WALK2_ONLYA | FSM_WALK2_ONLYB | FSM_WALK2_BOTH,
	FSM_WALK2_EDGE_INTERSECT = FSM_WALK2_BOTH,
	FSM_WALK2_EDGE_SUBTRACT  = FSM_WALK2_ONLYA | FSM_WALK2_BOTH
};

/* Constraints for end states for union (A|B), intersection (A&B), and
 * subtraction (A-B)
 *
 * Notice that the edge constraint for subtraction is different from the
 * end state constraint.  With subtraction, you want to follow edges
 * that are either ONLYA or BOTH, but valid end states must be ONLYA.
 */
enum {
	FSM_WALK2_END_UNION     = FSM_WALK2_ONLYA | FSM_WALK2_ONLYB | FSM_WALK2_BOTH,
	FSM_WALK2_END_INTERSECT = FSM_WALK2_BOTH,
	FSM_WALK2_END_SUBTRACT  = FSM_WALK2_ONLYA
};

struct bywalk_arena {
	struct state_tuple_pool *head;
	size_t top;

	struct fsm *new;
	struct set *states;

	/* table for which combinations are valid bits.
	 * There are four combinations:
	 *
	 *   a_end  b_end    AB		bit	endmask
	 *   false  false    00		0	0x1
	 *   false  true     01		1	0x2
	 *   true   false    10		2	0x4
	 *   true   true     11		3	0x8
	 *
	 * Here bit is the bit that expresses whether that combination
	 * is a valid end state or not.  We need four bits.
	 *
	 * Operation	Requirement			endmask
	 * intersect    both true			0x8
	 * subtract     first true, second false	0x4
	 * union	either true			0xE
	 */
	unsigned endmask:4;  /* bit table for what states are end states in the combined graph */
	unsigned edgemask:4; /* bit table for which edges should be followed */
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

static struct state_tuple *
alloc_state_tuple(struct bywalk_arena *ar)
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

static unsigned
walk2mask(int has_a, int has_b)
{
	int endbit = (!!has_a << 1) | !!has_b;
	return 1 << endbit;
}

static struct state_tuple *
new_state_tuple(struct bywalk_arena *ar, struct fsm_state *a, struct fsm_state *b)
{
	struct state_tuple lkup, *p;
	struct fsm_state *comb;
	const struct fsm_options *opt;
	int is_end; 

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

	is_end = ar->endmask & walk2mask(a && a->end, b && b->end);

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
walk_edges(struct bywalk_arena *ar, struct fsm *a, struct fsm *b, struct state_tuple *start);

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

	/* make sure inputs are DFAs */
	if (!fsm_all(a, fsm_isdfa)) {
		if (!fsm_determinise(a)) {
			goto error;
		}
	}

	if (!fsm_all(b, fsm_isdfa)) {
		if (!fsm_determinise(b)) {
			goto error;
		}
	}

	assert(fsm_all(a, fsm_isdfa));
	assert(fsm_all(b, fsm_isdfa));

	ar.new = fsm_new(a->opt);
	if (ar.new == NULL) {
		goto error;
	}

	ar.states = set_create(cmp_state_tuple);
	if (ar.states == NULL) {
		goto error;
	}

	ar.edgemask = FSM_WALK2_EDGE_INTERSECT;
	ar.endmask  = FSM_WALK2_END_INTERSECT;

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
	if (!walk_edges(&ar, a,b, tup0)) {
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

/* NULL-ify all the equiv members on the states */
static void
mark_equiv_null(struct fsm *fsm)
{
	struct fsm_state *src;

	assert(fsm != NULL);

	for (src = fsm->sl; src != NULL; src = src->next) {
		src->equiv = NULL;
	}
}

struct fsm *
fsm_subtract_bywalk(struct fsm *a, struct fsm *b)
{
	return fsm_subtract(a,b);
}

struct fsm *
fsm_subtract(struct fsm *a, struct fsm *b)
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

	/* make sure inputs are DFAs */
	if (!fsm_all(a, fsm_isdfa)) {
		if (!fsm_determinise(a)) {
			goto error;
		}
	}

	if (!fsm_all(b, fsm_isdfa)) {
		if (!fsm_determinise(b)) {
			goto error;
		}
	}

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

	ar.edgemask = FSM_WALK2_EDGE_SUBTRACT;
	ar.endmask  = FSM_WALK2_END_SUBTRACT;

	tup0 = new_state_tuple(&ar, sa,sb);
	if (tup0 == NULL) {
		goto error;
	}

	assert(tup0->a == sa);
	assert(tup0->b == sb);
	assert(tup0->comb != NULL);
        assert(tup0->comb->equiv == NULL); /* comb not yet been traversed */

	fsm_setstart(ar.new, tup0->comb);
	if (!walk_edges(&ar, a,b, tup0)) {
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

static int
walk_edges(struct bywalk_arena *ar, struct fsm *a, struct fsm *b, struct state_tuple *start)
{
	struct fsm_state *qa, *qb, *qc;
	struct set_iter ei, ej;
	const struct fsm_edge *ea, *eb;

	assert(a != NULL);
	assert(b != NULL);

	assert(ar->new != NULL);
	assert(ar->states != NULL);

	assert(start != NULL);

	/* This performs the actual walk by a depth-first search. */
	qa = start->a;
	qb = start->b;
	qc = start->comb;

	assert(qa != NULL || qb != NULL);
	assert(qc != NULL);

	/* fast exit if we've already visited the combined state */
	if (qc->equiv != NULL) {
		return 1;
	}

	/* mark combined state as visited */
	qc->equiv = qc;

	/* walk_edges walks the edges of two graphs, generating combined
	 * states.
	 *
	 * This is a synthesis of two separate walk functions, one for
	 * intersecting graphs and one for subtracting them.
	 *
	 * Basically, we need to provide some way to iterate over the
	 * cross-product of the states of both, but in a way that
	 * satisfies the operators.  There are two decision points:
	 *
	 *   1) whether to follow an edge to combined state (qa', qb'),
	 *      where either qa' or qb' may be NULL
	 *
	 *   2) whether a combined state (qa,qb) is an end state of the
	 *      two graphs, where either qa or qb may be NULL, and
	 *      either may be an end state.
	 *
	 * For each decision, there are four possible states and two
	 * possible outcomes (follow/don't-follow and end/not-end).
	 * These decisions can thus be compactly represented with two
	 * 4-bit tables.  The follow table is in ar->edgemask.  The end
	 * state table is in ar->endmask.
	 *
	 * There are two major loops, over the edges of A and over the
	 * edges of B.  In the first loop, we handle the ONLYA and BOTH
	 * cases.  In the second loop we handle the ONLYB cases.
	 */

        /* If qb == NULL, we can follow edges if ONLYA is allowed. */
	if (!qb && !(ar->edgemask & FSM_WALK2_ONLYA)) {
		return 1;
	}

        /* If qa == NULL, jump ahead to the ONLYB loop */
        if (!qa) {
		goto only_b;
	}

        /* If we can't follow ONLYA or BOTH edges, then jump ahead to
         * the ONLYB loop */
        if (!(ar->edgemask & (FSM_WALK2_BOTH|FSM_WALK2_ONLYA))) {
		goto only_b;
	}

	/* take care of only A and both A&B edges */
	for (ea = set_first(qa->edges, &ei); ea != NULL; ea=set_next(&ei)) {
		struct set_iter dia, dib;
		const struct fsm_state *da, *db;

		eb = qb ? fsm_hasedge(qb, ea->symbol) : NULL;

                /* If eb == NULL we can only follow this edge if ONLYA
                 * edges are allowed
                 */
		if (!eb && !(ar->edgemask & FSM_WALK2_ONLYA)) {
			continue;
		}

		for (da = set_first(ea->sl, &dia); da != NULL; da=set_next(&dia)) {
			db = eb ? set_first(eb->sl, &dib) : NULL;

			/* for loop with break to handle the situation where there is no
			 * corresponding edge in the B graph.  This will * proceed through
			 * the loop once, even when db == NULL.
			 */
			for (;;) {
				struct state_tuple *dst;

				/* FIXME: deal with annoying const-ness here */
				dst = new_state_tuple(ar, (struct fsm_state *)da, (struct fsm_state *)db);

				assert(dst != NULL);
				assert(dst->comb != NULL);

				if (!fsm_addedge(qc, dst->comb, ea->symbol)) {
					return 0;
				}

				/* depth-first traversal of the graph, but only traverse if the state has not
				 * yet been visited
				 */
				if (dst->comb->equiv == NULL) {
					if (!walk_edges(ar, a,b, dst)) {
						return 0;
					}
				}

				/* if db != NULL, fetch the next edge in B */
				if (db != NULL) {
					db = set_next(&dib);
				}

				/* if db == NULL, stop iterating over edges in B */
				if (db == NULL) {
					break;
				}
			}
		}
	}

only_b:
	/* fast exit if ONLYB cases aren't allowed */
	if (!qb || !(ar->edgemask & FSM_WALK2_ONLYB)) {
		return 1;
	}

	/* take care of only B edges */
	for (eb = set_first(qb->edges, &ej); eb != NULL; eb=set_next(&ej)) {
		struct set_iter dib;
		const struct fsm_state *db;

		ea = qa ? fsm_hasedge(qa, eb->symbol) : NULL;

		/* if A has the edge, it's not an only B edge */
		if (ea != NULL) {
			continue;
		}

		/* ONLYB loop is simpler because we only deal with
		 * states in the B graph (the A state is always NULL)
		 */
		for (db = set_first(eb->sl, &dib); db != NULL; db=set_next(&dib)) {
			for (;;) {
				struct state_tuple *dst;

				/* FIXME: deal with annoying const-ness here */
				dst = new_state_tuple(ar, NULL, (struct fsm_state *)db);

				assert(dst != NULL);
				assert(dst->comb != NULL);

				if (!fsm_addedge(qc, dst->comb, eb->symbol)) {
					return 0;
				}

				/* depth-first traversal of the graph, but only traverse if the state has not
				 * yet been visited
				 */
				if (dst->comb->equiv == NULL) {
					if (!walk_edges(ar, a,b, dst)) {
						return 0;
					}
				}
			}
		}
	}

	return 1;
}

