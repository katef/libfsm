/* $Id$ */

#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/out.h>
#include <fsm/options.h>

#include "internal.h"

static void
free_contents(struct fsm *fsm)
{
	struct fsm_state *next;
	struct fsm_state *s;

	assert(fsm != NULL);

	for (s = fsm->sl; s != NULL; s = next) {
		struct set_iter it;
		struct fsm_edge *e;
		next = s->next;

		for (e = set_first(s->edges, &it); e != NULL; e = set_next(&it)) {
			set_free(e->sl);
		}

		set_free(s->edges);
		free(s);
	}
}

struct fsm *
fsm_new(const struct fsm_options *opt)
{
	static const struct fsm_options defaults;
	struct fsm *new;

	if (opt == NULL) {
		opt = &defaults;
	}

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->sl    = NULL;
	new->tail  = &new->sl;
	new->start = NULL;

	new->endcount          = 0;
	new->may_have_epsilons = 0;

	new->opt = opt;

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

	if (dst->opt != src->opt) {
		errno = EINVAL;
		return; /* XXX */
	}

	free_contents(dst);

	dst->sl    = src->sl;
	dst->tail  = src->tail;
	dst->start = src->start;

	dst->endcount     = src->endcount;
	dst->has_epsilons = src->may_have_epsilons;

	free(src);
}

