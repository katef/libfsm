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
	struct fsm_state *state;
	unsigned int done:1;
	struct dlist *next;
};

struct dlist *
dlist_push(const struct fsm_alloc *a,
	struct dlist **list, struct fsm_state *state);

struct dlist *
dlist_nextnotdone(struct dlist *list);

int
dlist_contains(const struct dlist *list, const struct fsm_state *state);

void
dlist_free(const struct fsm_alloc *a, struct dlist *list);

#endif

