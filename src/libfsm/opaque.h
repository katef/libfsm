/* $Id$ */

#ifndef OPAQUE_H
#define OPAQUE_H


struct opaque_set {
	void *opaque;
	struct opaque_set *next;
};

struct opaque_set *
set_addopaque(struct opaque_set **head, void *opaque);

void
set_freeopaques(struct opaque_set *set);

#endif

