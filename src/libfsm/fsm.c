/* $Id$ */

#include <assert.h>
#include <stdlib.h>

#include <adt/set.h>

#include <fsm/fsm.h>

#include "internal.h"

static void
free_contents(struct fsm *fsm)
{
	struct fsm_state *s;
	struct fsm_state *next;
	int i;

	assert(fsm != NULL);

	for (s = fsm->sl; s != NULL; s = next) {
		next = s->next;

		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			set_free(s->edges[i].sl);
		}

		free(s);
	}
}

struct fsm *
fsm_new(void)
{
	struct fsm *new;

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->sl    = NULL;
	new->tail  = &new->sl;
	new->start = NULL;
	new->tidy  = 1;

#ifdef DEBUG_TODFA
	new->nfa   = NULL;
#endif

	return new;
}

void
fsm_free(struct fsm *fsm)
{
	assert(fsm != NULL);

	free_contents(fsm);

	free(fsm);
}

void
fsm_move(struct fsm *dst, struct fsm *src)
{
	assert(src != NULL);
	assert(dst != NULL);

	free_contents(dst);

	dst->sl    = src->sl;
	dst->tail  = src->tail;
	dst->start = src->start;

	free(src);
}

