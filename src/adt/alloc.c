/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include <adt/alloc.h>
#include <fsm/alloc.h>

void
f_free(const struct fsm_allocator *a, void *p)
{
	if (a == NULL) {
		free(p);
	} else {
		assert(a->free != NULL);
		a->free(a->opaque, p);
	}
}

void *
f_malloc(const struct fsm_allocator *a, size_t sz)
{
	if (a == NULL) {
		return malloc(sz);
	} else {
		assert(a->malloc != NULL);
		return a->malloc(a->opaque, sz);
	}
}

void *
f_realloc(const struct fsm_allocator *a, void *p, size_t sz)
{
	if (a == NULL) {
		return realloc(p, sz);
	} else {
		assert(a->realloc != NULL);
		return a->realloc(a->opaque, p, sz);
	}
}

