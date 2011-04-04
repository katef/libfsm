/* $Id$ */

#include <assert.h>
#include <stdlib.h>

#include "internal.h"
#include "opaque.h"

struct opaque_set *
set_addopaque(struct opaque_set **head, void *opaque)
{
	struct opaque_set *new;

	assert(head != NULL);

    /* Adding is a no-op for duplicate opaque values */
	{
		struct opaque_set *p;

		for (p = *head; p; p = p->next) {
			if (p->opaque == opaque) {
				return p;
			}
		}
	}

	/* Otherwise, add a new one */
	{
		new = malloc(sizeof *new);
		if (new == NULL) {
			return NULL;
		}

		new->opaque = opaque;

		new->next   = *head;
		*head       = new;
	}

	return new;
}

void
set_freeopaques(struct opaque_set *set)
{
	struct opaque_set *p;
	struct opaque_set *next;

	for (p = set; p; p = next) {
		next = p->next;
		free(p);
	}
}

