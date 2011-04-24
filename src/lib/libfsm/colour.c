/* $Id$ */

#include <assert.h>
#include <stdlib.h>

#include "internal.h"
#include "colour.h"

struct colour_set *
set_addcolour(const struct fsm *fsm, struct colour_set **head, void *colour)
{
	struct colour_set *new;

	assert(head != NULL);

    /* Adding is a no-op for duplicate colour values */
	{
		struct colour_set *c;

		for (c = *head; c != NULL; c = c->next) {
			if (colour_comp(fsm, c->colour, colour)) {
				return c;
			}
		}
	}

	/* Otherwise, add a new one */
	{
		new = malloc(sizeof *new);
		if (new == NULL) {
			return NULL;
		}

		new->colour = colour;

		new->next   = *head;
		*head       = new;
	}

	return new;
}

void
set_freecolours(struct colour_set *set)
{
	struct colour_set *c;
	struct colour_set *next;

	for (c = set; c != NULL; c = next) {
		next = c->next;
		free(c);
	}
}

int
colour_comp(const struct fsm *fsm, void *a, void *b)
{
	assert(fsm != NULL);

	if (fsm->comp == NULL) {
		return a == b;
	}

	return fsm->comp(fsm, a, b);
}

