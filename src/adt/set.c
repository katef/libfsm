/* $Id$ */

#include <assert.h>
#include <stdlib.h>

#include <adt/set.h>

struct state_set *
set_addstate(struct state_set **head, struct fsm_state *state)
{
	struct state_set *new;

	assert(head != NULL);
	assert(state != NULL);

	/* TODO: explain we find duplicate; return success */
	{
		struct state_set *s;

		for (s = *head; s != NULL; s = s->next) {
			if (s->state == state) {
				return s;
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
	struct state_set **s;

	assert(head != NULL);
	assert(state != NULL);

	for (s = head; *s != NULL; s = &(*s)->next) {
		assert((*s)->state != NULL);

		if ((*s)->state == state) {
			struct state_set *next;

			next = (*s)->next;
			free(*s);
			*s = next;
			break;
		}
	}

	assert(!set_contains(*head, state));
}

void
set_replace(struct state_set *set,
	struct fsm_state *old, struct fsm_state *new)
{
	struct state_set *s;

	assert(set != NULL);
	assert(old != NULL);
	assert(new != NULL);

	assert(!set_contains(set, new));

	for (s = set; s != NULL; s = s->next) {
		if (s->state == old) {
			s->state = new;
			break;
		}
	}

	assert(!set_contains(set, old));
}

void
set_free(struct state_set *set)
{
	struct state_set *s;
	struct state_set *next;

	for (s = set; s != NULL; s = next) {
		next = s->next;
		free(s);
	}
}

int
set_contains(const struct state_set *set, const struct fsm_state *state)
{
	const struct state_set *s;

	for (s = set; s != NULL; s = s->next) {
		if (s->state == state) {
			return 1;
		}
	}

	return 0;
}

int
subsetof(const struct state_set *a, const struct state_set *b)
{
	const struct state_set *s;

	for (s = a; s != NULL; s = s->next) {
		if (!set_contains(b, s->state)) {
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

void
set_merge(struct state_set **dst, struct state_set *src)
{
	struct state_set **p;

	assert(dst != NULL);

	for (p = dst; *p != NULL; p = &(*p)->next)
		;

	*p = src;
}

