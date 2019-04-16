/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdlib.h>

#include <adt/alloc.h>
#include <adt/set.h>
#include <adt/dlist.h>

/*
 * It might be more sensible to keep "visited" information in fsm_state
 * rather than going to the trouble of maintaining a list here. But for
 * the moment I want to keep such things self-contained.
 */

struct dlist *
dlist_push(const struct fsm_alloc *a,
	struct dlist **list, struct fsm_state *state)
{
	struct dlist *new;

	new = f_malloc(a, sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->done  = 0;
	new->state = state;

	new->next = *list;
	*list = new;

	return new;
}

struct dlist *
dlist_nextnotdone(struct dlist *list)
{
	struct dlist *p;

	for (p = list; p != NULL; p = p->next) {
		if (p->done) {
			continue;
		}

		return p;
	}

	return NULL;
}

int
dlist_contains(const struct dlist *list, const struct fsm_state *state)
{
	const struct dlist *p;

	for (p = list; p != NULL; p = p->next) {
		if (p->state == state) {
			return 1;
		}
	}

	return 0;
}

void
dlist_free(const struct fsm_alloc *a, struct dlist *list)
{
	struct dlist *p;
	struct dlist *next;

	for (p = list; p != NULL; p = next) {
		next = p->next;

		f_free(a, p);
	}
}

