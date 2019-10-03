/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ADT_PATH_H
#define ADT_PATH_H

struct fsm_alloc;
struct fsm_state;

struct path {
	fsm_state_t state;
	struct path *next;
	char c;
};

struct path *
path_push(const struct fsm_alloc *a,
	struct path **head, fsm_state_t state, char c);

void
path_free(const struct fsm_alloc *a, struct path *path);

#endif

