/* $Id$ */

#ifndef COLOUR_H
#define COLOUR_H

#include <stdio.h>

#include <fsm/colour.h>


struct colour_set {
	void *colour;
	struct colour_set *next;
};


struct colour_set *
set_addcolour(const struct fsm *fsm, struct colour_set **head, void *colour);

void
set_freecolours(struct colour_set *set);

int
set_containscolour(const struct fsm *fsm, void *colour, const struct colour_set *set);

int
colour_compare(const struct fsm *fsm, void *a, void *b);

#endif

