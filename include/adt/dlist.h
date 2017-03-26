/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef DLIST_H
#define DLIST_H

struct fsm_state;

struct dlist {
	struct fsm_state *state;
	unsigned int done:1;
	struct dlist *next;
};

struct dlist *
dlist_push(struct dlist **list, struct fsm_state *state);

struct dlist *
dlist_nextnotdone(struct dlist *list);

int
dlist_contains(const struct dlist *list, const struct fsm_state *state);

void
dlist_free(struct dlist *list);

#endif

