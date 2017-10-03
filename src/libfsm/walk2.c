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
#include "walk2.h"

/* XXX - revisit what would be a good size for this. */
enum { FSM_WALK2_TUPLE_POOL_SIZE = 1024 };

/* comparison of fsm_walk2_tuples for the (ordered) set */
static int
cmp_walk2_tuple(const void *a, const void *b)
{
	const struct fsm_walk2_tuple *pa = a, *pb = b;
	ptrdiff_t delta;

        /* XXX - do we need to specially handle NULLs? */

	delta = (pa->a > pb->a) - (pa->a < pb->a);
	if (delta == 0) {
		delta = (pa->b > pb->b) - (pa->b < pb->b);
	}

	return delta;
}

struct fsm_walk2_tuple_pool {
	struct fsm_walk2_tuple_pool *next;
	struct fsm_walk2_tuple items[FSM_WALK2_TUPLE_POOL_SIZE];
};

void
fsm_walk2_data_free(struct fsm_walk2_data *data)
{
	struct fsm_walk2_tuple_pool *p, *next;

	if (data->states) {
		set_free(data->states);
	}

	if (data->new) {
		fsm_free(data->new);
	}

	for (p = data->head; p != NULL; p = next) {
		next = p->next;
		free(p);
	}
}

static struct fsm_walk2_tuple *
alloc_walk2_tuple(struct fsm_walk2_data *data)
{
	static const struct fsm_walk2_tuple zero;

	struct fsm_walk2_tuple *item;
	struct fsm_walk2_tuple_pool *pool;

	if (data->head == NULL) {
		goto new_pool;
	}

	if (data->top >= (sizeof data->head->items / sizeof data->head->items[0])) {
		goto new_pool;
	}

	item = &data->head->items[data->top++];
	*item = zero;
	return item;

new_pool:
	pool = malloc(sizeof *pool);
	if (pool == NULL) {
		return NULL;
	}

	pool->next = data->head;
	data->head = pool;
	data->top = 1;

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

struct fsm_walk2_tuple *
fsm_walk2_tuple_new(struct fsm_walk2_data *data, struct fsm_state *a, struct fsm_state *b)
{
	struct fsm_walk2_tuple lkup, *p;
	struct fsm_state *comb;
	const struct fsm_options *opt;
	int is_end; 

	lkup.a = a;
	lkup.b = b;

	assert(data->states);

	p = set_contains(data->states, &lkup);
	if (p != NULL) {
		return p;
	}

	p = alloc_walk2_tuple(data);
	if (p == NULL) {
		return NULL;
	}

	comb = fsm_addstate(data->new);
	if (comb == NULL) {
		return NULL;
	}
	comb->equiv = NULL;

	p->a = a;
	p->b = b;
	p->comb = comb;
	if (!set_add(&data->states, p)) {
		return NULL;
	}

	is_end = data->endmask & walk2mask(a && a->end, b && b->end);

	if (is_end) {
		fsm_setend(data->new, comb, 1);

		opt = data->new->opt;
		if (opt != NULL && opt->carryopaque != NULL) {
			const struct fsm_state *states[2];
			states[0] = a;
			states[1] = b;
			/* this is slightly cheesed, but it avoids
			 * constructing a set just to pass these two
			 * states to the carryopaque function
			 */
			opt->carryopaque(states, 2, data->new, comb);
		}
	}

	return p;
}

/* NULL-ify all the equiv members on the states */
void
fsm_walk2_mark_equiv_null(struct fsm *fsm)
{
	struct fsm_state *src;

	assert(fsm != NULL);

	for (src = fsm->sl; src != NULL; src = src->next) {
		src->equiv = NULL;
	}
}

int
fsm_walk2_edges(struct fsm_walk2_data *data, struct fsm *a, struct fsm *b, struct fsm_walk2_tuple *start)
{
	struct fsm_state *qa, *qb, *qc;
	struct set_iter ei, ej;
	const struct fsm_edge *ea, *eb;

	assert(a != NULL);
	assert(b != NULL);

	assert(data->new != NULL);
	assert(data->states != NULL);

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

	/* fsm_walk2_edges walks the edges of two graphs, generating combined
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
	 * 4-bit tables.  The follow table is in data->edgemask.  The end
	 * state table is in data->endmask.
	 *
	 * There are two major loops, over the edges of A and over the
	 * edges of B.  In the first loop, we handle the ONLYA and BOTH
	 * cases.  In the second loop we handle the ONLYB cases.
	 */

        /* If qb == NULL, we can follow edges if ONLYA is allowed. */
	if (!qb && !(data->edgemask & FSM_WALK2_ONLYA)) {
		return 1;
	}

        /* If qa == NULL, jump ahead to the ONLYB loop */
        if (!qa) {
		goto only_b;
	}

        /* If we can't follow ONLYA or BOTH edges, then jump ahead to
         * the ONLYB loop */
        if (!(data->edgemask & (FSM_WALK2_BOTH|FSM_WALK2_ONLYA))) {
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
		if (!eb && !(data->edgemask & FSM_WALK2_ONLYA)) {
			continue;
		}

		for (da = set_first(ea->sl, &dia); da != NULL; da=set_next(&dia)) {
			db = eb ? set_first(eb->sl, &dib) : NULL;

			/* for loop with break to handle the situation where there is no
			 * corresponding edge in the B graph.  This will * proceed through
			 * the loop once, even when db == NULL.
			 */
			for (;;) {
				struct fsm_walk2_tuple *dst;

				/* FIXME: deal with annoying const-ness here */
				dst = fsm_walk2_tuple_new(data, (struct fsm_state *)da, (struct fsm_state *)db);

				assert(dst != NULL);
				assert(dst->comb != NULL);

				if (!fsm_addedge(qc, dst->comb, ea->symbol)) {
					return 0;
				}

				/* depth-first traversal of the graph, but only traverse if the state has not
				 * yet been visited
				 */
				if (dst->comb->equiv == NULL) {
					if (!fsm_walk2_edges(data, a,b, dst)) {
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
	if (!qb || !(data->edgemask & FSM_WALK2_ONLYB)) {
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
				struct fsm_walk2_tuple *dst;

				/* FIXME: deal with annoying const-ness here */
				dst = fsm_walk2_tuple_new(data, NULL, (struct fsm_state *)db);

				assert(dst != NULL);
				assert(dst->comb != NULL);

				if (!fsm_addedge(qc, dst->comb, eb->symbol)) {
					return 0;
				}

				/* depth-first traversal of the graph, but only traverse if the state has not
				 * yet been visited
				 */
				if (dst->comb->equiv == NULL) {
					if (!fsm_walk2_edges(data, a,b, dst)) {
						return 0;
					}
				}
			}
		}
	}

	return 1;
}

struct fsm *
fsm_walk2(struct fsm *a, struct fsm *b, unsigned edgemask, unsigned endmask)
{
	static const struct fsm_walk2_data zero;
	struct fsm_walk2_data data = zero;

	struct fsm *new = NULL;
	struct fsm_state *sa, *sb;
	struct fsm_walk2_tuple *tup0;

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

	sa = fsm_getstart(a);
	sb = fsm_getstart(b);

	if (!sa && !sb) {
		/* if both A and B lack a start states, the
		 * result will be empty */
		goto finish;
	}

	if (sb == NULL) {
		if (endmask & FSM_WALK2_ONLYA) {
			/* must be a copy of A */
			return fsm_clone(a);
		} 
		/* !sb and combined states cannot be ONLYA, so the
		 * result will be empty 
		 */
		goto finish;
	}

	if (sa == NULL) {
		if (endmask & FSM_WALK2_ONLYB) {
			/* must be a copy of B */
			return fsm_clone(b);
		}
		/* !sa and combined states cannot be ONLYB, so the
		 * result will be empty 
		 */
		goto finish;
	}

	data.edgemask = edgemask;
	data.endmask  = endmask;

	data.new = fsm_new(a->opt);
	if (data.new == NULL) {
		goto error;
	}

	data.states = set_create(cmp_walk2_tuple);
	if (data.states == NULL) {
		goto error;
	}

	tup0 = fsm_walk2_tuple_new(&data, sa,sb);
	if (tup0 == NULL) {
		goto error;
	}

	assert(tup0->a == sa);
	assert(tup0->b == sb);
	assert(tup0->comb != NULL);
	assert(tup0->comb->equiv == NULL); /* comb not yet been traversed */

	fsm_setstart(data.new, tup0->comb);
	if (!fsm_walk2_edges(&data, a,b, tup0)) {
		goto error;
	}

finish:
	new = data.new;
	data.new = NULL; /* avoid freeing new FSM */

	/* reset all equiv fields in the states */
        fsm_walk2_mark_equiv_null(new);

	fsm_walk2_data_free(&data);
	return new;

error:
	fsm_walk2_data_free(&data);
	return NULL;
}

