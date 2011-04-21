/* $Id$ */

#include <assert.h>
#include <stdlib.h>

#include "internal.h"
#include "set.h"

struct state_set *
set_addstate(struct state_set **head, struct fsm_state *state)
{
	struct state_set *new;

	assert(head != NULL);
	assert(state != NULL);

	/* TODO: explain we find duplicate; return success */
	{
		struct state_set *p;

		for (p = *head; p; p = p->next) {
			if (p->state == state) {
				return p;
			}
		}
	}

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->state = state;

	new->next  = *head;
	*head      = new;

	return new;
}

void
set_remove(struct state_set **head, struct fsm_state *state)
{
	struct state_set **p;
	struct state_set *next;

	assert(head != NULL);
	assert(state != NULL);

	for (p = head; *p; p = &(*p)->next) {
		next = (*p)->next;

		free(*p);

		*p = next;

		assert(!set_contains(state, *head));
		return;
	}
}

void
set_free(struct state_set *set)
{
	struct state_set *p;
	struct state_set *next;

	for (p = set; p; p = next) {
		next = p->next;
		free(p);
	}
}

int
set_containsendstate(struct fsm *fsm, struct state_set *set)
{
	struct state_set *s;

	assert(fsm != NULL);

	for (s = set; s; s = s->next) {
		/* TODO: no need to pass fsm here */
		if (fsm_isend(fsm, s->state)) {
			return 1;
		}
	}

	return 0;
}

int
set_contains(const struct fsm_state *state, const struct state_set *set)
{
	const struct state_set *p;

	for (p = set; p; p = p->next) {
		if (p->state == state) {
			return 1;
		}
	}

	return 0;
}

/* Find if a is a subset of b */
static int subsetof(const struct state_set *a, const struct state_set *b) {
	const struct state_set *p;

	for (p = a; p; p = p->next) {
		if (!set_contains(p->state, b)) {
			return 0;
		}
	}

	return 1;
}

int
set_equal(const struct state_set *a, const struct state_set *b)
{
	return subsetof(a, b) && subsetof(b, a);
}

