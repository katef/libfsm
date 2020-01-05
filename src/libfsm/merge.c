/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>

#include <adt/set.h>
#include <adt/edgeset.h>
#include <adt/stateset.h>

#include "internal.h"

static struct fsm *
merge(struct fsm *dst, struct fsm *src,
	fsm_state_t *base_dst, fsm_state_t *base_src)
{
	assert(dst != NULL);
	assert(src != NULL);
	assert(base_src != NULL);
	assert(base_dst != NULL);

	if (dst->statealloc < src->statecount + dst->statecount) {
		void *tmp;

		size_t newalloc = src->statecount + dst->statecount;

		/* TODO: round up to next power of two here?
		 * or let realloc do that internally */
		tmp = f_realloc(dst->opt->alloc, dst->states, newalloc * sizeof *dst->states);
		if (tmp == NULL) {
			return NULL;
		}

		dst->states = tmp;
		dst->statealloc = newalloc;
	}

	/*
	 * XXX: base_a and base_b would become redundant if we change to the
	 * shared global array idea.
	 */
	{
		fsm_state_t i;

		*base_dst = 0;
		*base_src = dst->statecount;

		for (i = 0; i < src->statecount; i++) {
			state_set_rebase(&src->states[i].epsilons, *base_src);
			edge_set_rebase(&src->states[i].edges, *base_src);
		}
	}

	memcpy(dst->states + dst->statecount, src->states,
		src->statecount * sizeof *src->states);
	dst->statecount += src->statecount;
	dst->endcount   += src->endcount;

	f_free(src->opt->alloc, src->states);
	src->states = NULL;
	src->statealloc = 0;
	src->statecount = 0;

	fsm_clearstart(src);
	fsm_clearstart(dst);

	fsm_free(src);

	return dst;
}

struct fsm *
fsm_mergeab(struct fsm *a, struct fsm *b,
	fsm_state_t *base_b)
{
	fsm_state_t dummy; /* always 0 */
	struct fsm *q;

	assert(a != NULL);
	assert(b != NULL);
	assert(base_b != NULL);

	if (a->opt != b->opt) {
		errno = EINVAL;
		return NULL;
	}

	/*
	 * We merge b into a.
	 */

	q = merge(a, b, &dummy, base_b);

	assert(dummy == 0);

	return q;
}

struct fsm *
fsm_merge(struct fsm *a, struct fsm *b,
	fsm_state_t *base_a, fsm_state_t *base_b)
{
	assert(a != NULL);
	assert(b != NULL);
	assert(base_a != NULL);
	assert(base_b != NULL);

	if (a->opt != b->opt) {
		errno = EINVAL;
		return NULL;
	}

	/*
	 * We merge the smaller FSM into the larger FSM.
	 * The number of allocate states is considered first, because realloc
	 * is probably more expensive than memcpy.
	 */

	if (a->statealloc > b->statealloc) {
		return merge(a, b, base_a, base_b);
	} else if (a->statealloc < b->statealloc) {
		return merge(b, a, base_b, base_a);
	}

	if (a->statecount > b->statecount) {
		return merge(a, b, base_a, base_b);
	} else if (a->statecount < b->statecount) {
		return merge(b, a, base_b, base_a);
	}

	return merge(a, b, base_a, base_b);
}

