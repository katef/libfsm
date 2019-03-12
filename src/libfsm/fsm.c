/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include <adt/set.h>

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
		struct set_iter it;
		struct fsm_edge *e;
		next = s->next;

		for (e = set_first(s->edges, &it); e != NULL; e = set_next(&it)) {
			set_free(e->sl);
			f_free(fsm, e);
		}

		set_free(s->edges);
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
fsm_carryopaque(struct fsm *fsm, const struct set *set,
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

	fsm->opt->carryopaque((void *) set_array(set), set_count(set),
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
		struct set_iter ei;
		const struct fsm_edge *e;

		for (e = set_first(src->edges, &ei); e != NULL; e=set_next(&ei)) {
			assert(n + set_count(e->sl) > n); /* handle overflow with more grace? */
			n += set_count(e->sl);
		}
	}

	return n;
}
