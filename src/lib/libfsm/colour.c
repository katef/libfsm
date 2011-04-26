/* $Id$ */

#include <assert.h>
#include <stdlib.h>

#include "internal.h"
#include "colour.h"


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
set_containscolour(const struct fsm *fsm, void *colour, const struct colour_set *set)
{
	const struct colour_set *s;

	for (s = set; s != NULL; s = s->next) {
		if (colour_compare(fsm, s->colour, colour)) {
			return 1;
		}
	}

	return 0;
}

int
colour_compare(const struct fsm *fsm, void *a, void *b)
{
	assert(fsm != NULL);

	if (fsm->colour_hooks.compare == NULL) {
		return a == b;
	}

	return fsm->colour_hooks.compare(fsm, a, b);
}

struct colour_set *
set_addcolour(const struct fsm *fsm, struct colour_set **head, void *colour)
{
	struct colour_set *new;

	assert(head != NULL);

    /* Adding is a no-op for duplicate colour values */
	{
		struct colour_set *c;

		for (c = *head; c != NULL; c = c->next) {
			if (colour_compare(fsm, c->colour, colour)) {
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

int
fsm_addstatecolour(struct fsm *fsm, struct fsm_state *state, void *colour)
{
	assert(fsm != NULL);
	assert(state != NULL);

	return !!set_addcolour(fsm, &state->cl, colour);
}

int
fsm_addcolour(struct fsm *fsm, void *colour)
{
	struct fsm_state *s;

	assert(fsm != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		if (!set_addcolour(fsm, &s->cl, colour)) {
			return 0;
		}
	}

	return 1;
}

void
fsm_setcolourhooks(struct fsm *fsm, const struct fsm_colour_hooks *h)
{
	static const struct fsm_colour_hooks default_hooks;

	assert(fsm != NULL);

	if (h == NULL) {
		fsm->colour_hooks = default_hooks;
	} else {
		fsm->colour_hooks = *h;
	}
}

