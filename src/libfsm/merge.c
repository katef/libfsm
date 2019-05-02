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

#include <fsm/bool.h>

#include "internal.h"

static struct fsm *
merge(struct fsm *src, struct fsm *dst)
{
	assert(src != NULL);
	assert(dst != NULL);

	if (dst->statealloc < src->statecount + dst->statecount) {
		void *tmp;

		tmp = f_realloc(dst->opt->alloc, dst->states,
			(src->statecount + dst->statecount) * sizeof *dst->states);
		if (tmp == NULL) {
			return NULL;
		}

		dst->states = tmp;
		dst->statealloc += src->statealloc;
	}

	memcpy(dst->states + dst->statecount, src->states,
		src->statecount * sizeof *src->states);
	dst->statecount += src->statecount;

	f_free(src->opt->alloc, src->states);
	src->states = NULL;
	src->statealloc = 0;
	src->statecount = 0;

	dst->endcount += src->endcount;

	fsm_setstart(src, NULL);
	fsm_setstart(dst, NULL);

	fsm_free(src);

	return dst;
}

struct fsm *
fsm_merge(struct fsm *a, struct fsm *b)
{
	assert(a != NULL);
	assert(b != NULL);

	if (a->opt != b->opt) {
		errno = EINVAL;
		return NULL;
	}

	if (a->statealloc >= b->statealloc) {
		return merge(b, a);
	} else {
		return merge(a, b);
	}
}

