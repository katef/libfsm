/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ALLOC_H
#define ALLOC_H

struct fsm_alloc;

/*
 * Internal allocation functions which invoke malloc(3), free(3) etc by default,
 * or equivalent user-provided functions to allocate and free memory and perform
 * any custom memory tracking or handling.
 */

void
f_free(const struct fsm_alloc *a, void *p);

void *
f_calloc(const struct fsm_alloc *a, size_t n, size_t sz);

void *
f_malloc(const struct fsm_alloc *a, size_t sz);

void *
f_realloc(const struct fsm_alloc *a, void *p, size_t sz);

#endif

