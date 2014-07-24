/* $Id$ */

#include <assert.h>
#include <stdlib.h>

#include <adt/path.h>

struct path *
path_push(struct path **head, struct fsm_state *state, int type)
{
	struct path *new;

	assert(head != NULL);
	assert(state != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->state = state;
	new->type  = type;

	new->next  = *head;
	*head      = new;

	return new;
}

void
path_free(struct path *path)
{
	struct path *p;
	struct path *next;

	for (p = path; p != NULL; p = next) {
		next = p->next;
		free(p);
	}
}

