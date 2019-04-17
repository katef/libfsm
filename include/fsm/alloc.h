/*
 * Copyright 2008-2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef FSM_ALLOC_H
#define FSM_ALLOC_H

struct fsm_alloc {
	void (*free)(void *opaque, void *p);
	void *(*calloc)(void *opaque, size_t n, size_t sz);
	void *(*malloc)(void *opaque, size_t sz);
	void *(*realloc)(void *opaque, void *p, size_t sz);
	void *opaque;
};

#endif

