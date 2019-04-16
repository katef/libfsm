/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <adt/alloc.h>
#include <adt/path.h>

struct path *
path_push(const struct fsm_alloc *a,
	struct path **head, struct fsm_state *state, char c)
{
	struct path *new;

	assert(head != NULL);
	assert(state != NULL);

	new = f_malloc(a, sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->state = state;
	new->c     = c;

	new->next  = *head;
	*head      = new;

	return new;
}

void
path_free(const struct fsm_alloc *a, struct path *path)
{
	struct path *p;
	struct path *next;

	for (p = path; p != NULL; p = next) {
		next = p->next;
		f_free(a, p);
	}
}

