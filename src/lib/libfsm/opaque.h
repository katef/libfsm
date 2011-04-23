/* $Id$ */

#ifndef OPAQUE_H
#define OPAQUE_H


struct opaque_set {
	void *opaque;
	struct opaque_set *next;
};

struct opaque_set *
set_addopaque(const struct fsm *fsm, struct opaque_set **head, void *opaque);

void
set_freeopaques(struct opaque_set *set);

int
opaque_comp(const struct fsm *fsm, void *a, void *b);

#endif

