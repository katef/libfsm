/* $Id$ */

#include <assert.h>
#include <stdlib.h>
#include <stddef.h>

#include <adt/set.h>
#include <adt/xalloc.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/walk.h>

#include "../internal.h"

struct list {
	struct fsm_state *state;
	unsigned int done:1;
	struct list *next;
};

static struct list *
list_push(struct list **list, struct fsm_state *state)
{
	struct list *new;

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->done  = 0;
	new->state = state;

	new->next = *list;
	*list = new;

	return new;
}

static struct list *
list_nextnotdone(struct list *list)
{
	struct list *p;

	for (p = list; p != NULL; p = p->next) {
		if (p->done) {
			continue;
		}

		return p;
	}

	return NULL;
}

static int
list_contains(const struct list *list, const struct fsm_state *state)
{
	const struct list *p;

	for (p = list; p != NULL; p = p->next) {
		if (p->state == state) {
			return 1;
		}
	}

	return 0;
}

static void
list_free(struct list *list)
{
	struct list *p;
	struct list *next;

	for (p = list; p != NULL; p = next) {
		next = p->next;

		free(p);
	}
}

int
fsm_reachable(struct fsm *fsm, struct fsm_state *state,
	int (*predicate)(const struct fsm *, const struct fsm_state *))
{
	struct list *list;
	struct list *p;

	assert(state != NULL);
	assert(predicate != NULL);

	/*
	 * Iterative depth-first search.
	 *
	 * It might be more sensible to keep "visited" information in fsm_state
	 * rather than going to the trouble of maintaining a list here. But for
	 * the moment I want to keep such things self-contained.
	 */
	/* TODO: write in terms of fsm_walk or some common iteration callback */

	list = NULL;

	if (!list_push(&list, state)) {
		return -1;
	}

	while (p = list_nextnotdone(list), p != NULL) {
		struct fsm_edge *e;
		struct set_iter it;

		if (!predicate(fsm, p->state)) {
			list_free(p);
			return 0;
		}

		for (e = set_first(p->state->edges, &it); e != NULL; e = set_next(&it)) {
			struct fsm_state *st;
			struct set_iter jt;

			for (st = set_first(e->sl, &jt); st != NULL; st = set_next(&jt)) {
				/* not a list operation... */
				if (list_contains(list, st)) {
					continue;
				}

				if (!list_push(&list, st)) {
					return -1;
				}
			}
		}

		p->done = 1;
	}

	list_free(list);

	return 1;
}

