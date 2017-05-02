/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include <adt/set.h>

#include <fsm/bool.h>
#include <fsm/out.h>
#include <fsm/options.h>

#include "internal.h"

struct fsm *
fsm_merge(struct fsm *a, struct fsm *b)
{
	assert(a != NULL);
	assert(b != NULL);

	if (a->opt != b->opt) {
		errno = EINVAL;
		return NULL;
	}

	if (b->opt != NULL && b->opt->tags) {
		struct fsm_state *s;
		struct fsm_edge *e;
		struct set_iter it;

		if (TAG_MAX - a->tagcount < b->tagcount) {
			errno = E2BIG;
			return NULL;
		}

		for (s = b->sl; s != NULL; s = s->next) {
			for (e = set_first(s->edges, &it); e != NULL; e = set_next(&it)) {
				e->tags <<= a->tagcount;
			}
		}
	}

	*a->tail = b->sl;
	a->tail  = b->tail;

	a->endcount += b->endcount;
	a->tagcount += b->tagcount;

	b->sl   = NULL;
	b->tail = NULL;
	fsm_free(b);

	fsm_setstart(a, NULL);

	return a;
}

