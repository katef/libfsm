/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include <fsm/alloc.h>
#include <fsm/options.h>

#include "internal.h"

#define ctassert(pred) \
	switch (0) { case 0: case (pred):; }

void
f_free(const struct fsm *fsm, void *p)
{
	const struct fsm_allocator *a;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	a = fsm->opt->allocator;

	if (a == NULL) {
		free(p);
	} else {
		assert(a->free != NULL);
		a->free(a->opaque, p);
	}
}

void *
f_malloc(const struct fsm *fsm, size_t sz)
{
	const struct fsm_allocator *a;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	a = fsm->opt->allocator;

	if (a == NULL) {
		return malloc(sz);
	} else {
		assert(a->malloc != NULL);
		return a->malloc(a->opaque, sz);
	}
}

void *
f_realloc(const struct fsm*fsm, void *p, size_t sz)
{
	const struct fsm_allocator *a;

	assert(fsm != NULL);
	assert(fsm->opt != NULL);

	a = fsm->opt->allocator;

	if (a == NULL) {
		return realloc(p, sz);
	} else {
		assert(a->realloc != NULL);
		return a->realloc(a->opaque, p, sz);
	}
}

