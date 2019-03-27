/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include <adt/set.h>
#include <adt/stateset.h>
#include <adt/edgeset.h>

#include <fsm/fsm.h>
#include <fsm/print.h>
#include <fsm/options.h>

#include "internal.h"

#define ctassert(pred) \
	switch (0) { case 0: case (pred):; }

void
f_free(const struct fsm *fsm, void *p)
{
	struct fsm_allocator a;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	a = fsm->opt->allocator;
	if (a.free != NULL) {
		a.free(a.opaque, p);
		return;
	}

	free(p);
}

void *
f_malloc(const struct fsm *fsm, size_t sz)
{
	struct fsm_allocator a;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	a = fsm->opt->allocator;
	if (a.malloc != NULL) {
		return (a.malloc(a.opaque, sz));
	}

	return malloc(sz);
}

void *
f_realloc(const struct fsm*fsm, void *p, size_t sz)
{
	struct fsm_allocator a;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	a = fsm->opt->allocator;
	if (a.realloc != NULL) {
		return a.realloc(a.opaque, p, sz);
	}

	return realloc(p, sz);
}


void
free_contents(struct fsm *fsm)
{
	struct fsm_state *next;
	struct fsm_state *s;

	assert(fsm != NULL);

	for (s = fsm->sl; s != NULL; s = next) {
		struct edge_iter it;
		struct fsm_edge *e;
		next = s->next;

		for (e = edge_set_first(s->edges, &it); e != NULL; e = edge_set_next(&it)) {
			state_set_free(e->sl);
			f_free(fsm, e);
		}

		edge_set_free(s->edges);
		f_free(fsm, s);
	}
}

struct fsm *
fsm_new(const struct fsm_options *opt)
{
	static const struct fsm_options defaults;
	struct fsm *new, f;

	if (opt == NULL) {
		opt = &defaults;
	}

	f.opt = opt;

	new = f_malloc(&f, sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->sl    = NULL;
	new->tail  = &new->sl;
	new->start = NULL;

	new->endcount = 0;

	new->opt = opt;

#ifdef DEBUG_TODFA
	new->nfa   = NULL;
#endif

	return new;
}

void
fsm_free(struct fsm *fsm)
{
	assert(fsm != NULL);

	free_contents(fsm);

	f_free(fsm, fsm);
}

const struct fsm_options *
fsm_getoptions(const struct fsm *fsm)
{
	return fsm->opt;
}

void
fsm_setoptions(struct fsm *fsm, const struct fsm_options *opts)
{
	fsm->opt = opts;
}

void
fsm_move(struct fsm *dst, struct fsm *src)
{
	assert(src != NULL);
	assert(dst != NULL);

	if (dst->opt != src->opt) {
		errno = EINVAL;
		return; /* XXX */
	}

	free_contents(dst);

	dst->sl       = src->sl;
	dst->tail     = src->tail;
	dst->start    = src->start;
	dst->endcount = src->endcount;

	f_free(src, src);
}

void
fsm_carryopaque(struct fsm *fsm, const struct state_set *set,
	struct fsm *new, struct fsm_state *state)
{
	ctassert(sizeof (void *) == sizeof (struct fsm_state *));

	assert(fsm != NULL);

	if (fsm->opt == NULL || fsm->opt->carryopaque == NULL) {
		return;
	}

	/*
	 * Our sets are a void ** treated as an array of elements of type void *.
	 * Here we're presenting these as if they're an array of elements
	 * of type struct fsm_state *.
	 *
	 * This is not portable because the representations of those types
	 * need not be compatible in general, hence the compile-time assert
	 * and the cast here.
	 */

	fsm->opt->carryopaque((void *) state_set_array(set), state_set_count(set),
		new, state);
}

unsigned int
fsm_countstates(const struct fsm *fsm)
{
	unsigned int n = 0;
	const struct fsm_state *s;
	/*
	 * XXX - this walks the list and counts and should be replaced
	 * with something better when possible
	 */
	for (s = fsm->sl; s != NULL; s = s->next) {
		assert(n+1>n); /* handle overflow with more grace? */
		n++;
	}

	return n;
}

unsigned int
fsm_countedges(const struct fsm *fsm)
{
	unsigned int n = 0;
	const struct fsm_state *src;

	/*
	 * XXX - this counts all src,lbl,dst tuples individually and
	 * should be replaced with something better when possible
	 */
	for (src = fsm->sl; src != NULL; src = src->next) {
		struct edge_iter ei;
		const struct fsm_edge *e;

		for (e = edge_set_first(src->edges, &ei); e != NULL; e=edge_set_next(&ei)) {
			assert(n + state_set_count(e->sl) > n); /* handle overflow with more grace? */
			n += state_set_count(e->sl);
		}
	}

	return n;
}

static void
clear_states(struct fsm_state *s)
{
	for (; s != NULL; s = s->next) { s->reachable = 0; }
}

static void
mark_states(struct fsm_state *s)
{
	const struct fsm_edge *e;
	struct edge_iter edge_iter;

	/* skip if already processed */
	if (s->reachable) { return; }
	s->reachable = 1;
	
	for (e = edge_set_first(s->edges, &edge_iter); e != NULL; e = edge_set_next(&edge_iter)) {
		struct state_iter state_iter;
		struct fsm_state *es;
		for (es = state_set_first(e->sl, &state_iter); es != NULL; es = state_set_next(&state_iter)) {
			/* TODO: A long run of transitions could potentially overrun the stack
			 * here. It should probably use some sort of queue. If struct fsm
			 * tracked the total number of states in `sl`, then it could allocate
			 * the worst-case queue upfront, or fail cleanly.
			 *
			 * Since `struct fsm_state` contains `equiv`, which is only meaningful
			 * during a different operation, that could be moved inside a union
			 * and write a backward link into that field. */
			mark_states(es);
 		}
	}
}

void
sweep_states(struct fsm *fsm)
{
	struct fsm_state *prev = NULL;
	struct fsm_state *s = fsm->sl;
	struct fsm_state *next;
	/* This doesn't use fsm_removestate because it would be modifying the
	 * state graph while traversing it, and any state being removed here
	 * should (by definition) not be the start, or have any other reachable
	 * edges referring to it.
	 *
	 * There may temporarily be other states in the graph with other
	 * to it, because the states aren't topologically sorted, but
	 * they'll be collected soon as well. */
	while (s != NULL) {
		next = s->next;
		if (!s->reachable) {
			assert(s != fsm->start);

			/* for endcount accounting */
			fsm_setend(fsm, s, 0);

			/* unlink */
			if (prev != NULL) { prev->next = next; }
			if (s == *fsm->tail) { *fsm->tail = prev; }
			edge_set_free(s->edges);
			free(s);
		} else {
			prev = s;
		}
		s = next;
	}
}

void
fsm_collect_unreachable_states(struct fsm *fsm)
{
	clear_states(fsm->sl);
	mark_states(fsm->start);
	sweep_states(fsm);
}
