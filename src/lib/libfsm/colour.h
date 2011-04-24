/* $Id$ */

#ifndef COLOUR_H
#define COLOUR_H


struct colour_set {
	void *colour;
	struct colour_set *next;
};

struct colour_set *
set_addcolour(const struct fsm *fsm, struct colour_set **head, void *opaque);

void
set_freecolours(struct colour_set *set);

int
colour_comp(const struct fsm *fsm, void *a, void *b);

#endif

