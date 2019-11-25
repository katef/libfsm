/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef ADT_DLIST_H
#define ADT_DLIST_H

struct fsm_alloc;
struct fsm_state;

struct dlist {
	fsm_state_t state;
	unsigned int done:1;
	struct dlist *next;
};

struct dlist *
dlist_push(const struct fsm_alloc *a,
	struct dlist **list, fsm_state_t state);

struct dlist *
dlist_nextnotdone(struct dlist *list);

int
dlist_contains(const struct dlist *list, fsm_state_t state);

void
dlist_free(const struct fsm_alloc *a, struct dlist *list);

#endif

