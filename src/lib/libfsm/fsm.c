/* $Id$ */

#include <assert.h>
#include <stdlib.h>

#include <fsm/fsm.h>
#include <fsm/colour.h>

#include "internal.h"
#include "set.h"
#include "colour.h"

static unsigned int inventid(const struct fsm *fsm) {
	assert(fsm != NULL);

	return fsm_getmaxid(fsm) + 1;
}

static void free_contents(struct fsm *fsm) {
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
	static const struct fsm_options default_options;
	struct fsm *new;

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->sl      = NULL;
	new->start   = NULL;
	new->options = default_options;

	fsm_setcolourhooks(new, NULL);

	return new;
}

void
fsm_free(struct fsm *fsm)
{
	assert(fsm != NULL);

	free_contents(fsm);

	free(fsm);
}

struct fsm *
fsm_copy(struct fsm *fsm)
{
	struct fsm_state *s;
	struct fsm *new;

	new = fsm_new();
	if (new == NULL) {
		return NULL;
	}

	/* recreate states */
	for (s = fsm->sl; s != NULL; s = s->next) {
		struct fsm_state *state;
		struct colour_set *c;

		state = fsm_addstateid(new, s->id);
		if (state == NULL) {
			fsm_free(new);
			return NULL;
		}

		for (c = s->cl; c != NULL; c = c->next) {
			if (!fsm_addstatecolour(new, state, c->colour)) {
				fsm_free(new);
				return NULL;
			}
		}

		if (fsm_isend(fsm, s)) {
			fsm_setend(new, state, 1);
		}
	}

	/* recreate edges */
	for (s = fsm->sl; s != NULL; s = s->next) {
		struct state_set *e;
		struct fsm_state *from;
		int i;

		from = fsm_getstatebyid(new, s->id);

		/* TODO: centralise this with reverse.c */
		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			for (e = s->edges[i].sl; e != NULL; e = e->next) {
				struct fsm_state *to;

				assert(e->state != NULL);

				to = fsm_getstatebyid(new, e->state->id);

				assert(from != NULL);
				assert(to   != NULL);

				switch (i) {
				case FSM_EDGE_EPSILON:
					if (!fsm_addedge_epsilon(new, from, to)) {
						fsm_free(new);
						return NULL;
					}
					break;

				default:
					assert(i >= 0);
					assert(i <= UCHAR_MAX);

					if (!fsm_addedge_literal(new, from, to, i)) {
						fsm_free(new);
						return NULL;
					}
				}
			}
		}
	}

	new->start = fsm_getstatebyid(new, fsm->start->id);
	if (new->start == NULL) {
		fsm_free(new);
		return NULL;
	}

	new->options = fsm->options;

	return new;
}

void
fsm_move(struct fsm *dst, struct fsm *src)
{
	assert(src != NULL);
	assert(dst != NULL);

	free_contents(dst);

	dst->sl      = src->sl;
	dst->start   = src->start;
	dst->options = src->options;

	free(src);
}

int
fsm_union(struct fsm *dst, struct fsm *src)
{
	struct fsm_state **sl;
	struct fsm_state *s;

	assert(dst != NULL);
	assert(src != NULL);

	/*
	 * Splice over lists from src to dst.
	 */

	for (sl = &dst->sl; *sl != NULL; sl = &(*sl)->next) {
		/* nothing */
	}
	*sl = src->sl;


	/*
	 * Add epsilon transition from dst's start state to src's start state.
	 * XXX: i believe this is incorrect when dst has a transition to dst->start.
	 */
	if (src->start != NULL) {
		if (dst->start == NULL) {
			dst->start = src->start;
		} else {
			struct colour_set *c;

			for (c = src->start->cl; c != NULL; c = c->next) {
				if (!fsm_addstatecolour(dst, dst->start, c->colour)) {
					/* TODO: this leaves dst in a questionable state */
					return 0;
				}
			}

			if (!fsm_addedge_epsilon(dst, dst->start, src->start)) {
				/* TODO: this leaves dst in a questionable state */
				return 0;
			}
		}
	} else {
		/* TODO: src has no start state. rethink during API refactor */
	}


	/*
	 * Renumber incoming states so as not to clash over existing ones.
	 */
	for (s = src->sl; s != NULL; s = s->next) {
		s->id = 0;
	}
	for (s = src->sl; s != NULL; s = s->next) {
		s->id = inventid(dst);
	}


	free(src);

	return 1;
}

struct fsm_state *
fsm_addstateid(struct fsm *fsm, unsigned int id)
{
	struct fsm_state *new;

	assert(fsm != NULL);

	/* Find an existing state */
	{
		struct fsm_state *s;

		for (s = fsm->sl; s; s = s->next) {
			if (s->id == id) {
				return s;
			}
		}
	}

	/* Otherwise, create a new one */
	{
		int i;

		new = malloc(sizeof *new);
		if (new == NULL) {
			return NULL;
		}

		new->id  = id;
		new->end = 0;
		new->cl  = NULL;

		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			new->edges[i].sl = NULL;
			new->edges[i].cl = NULL;
		}

		new->next = fsm->sl;
		fsm->sl = new;
	}

	return new;
}

struct fsm_state *
fsm_addstate(struct fsm *fsm)
{
	assert(fsm != NULL);

	return fsm_addstateid(fsm, inventid(fsm));
}

void
fsm_setoptions(struct fsm *fsm, const struct fsm_options *options)
{
	assert(fsm != NULL);
	assert(options != NULL);

	fsm->options = *options;
}

void
fsm_setend(struct fsm *fsm, struct fsm_state *state, int end)
{
	(void) fsm;

	assert(fsm != NULL);
	assert(state != NULL);

	state->end = end;
}

int
fsm_isend(const struct fsm *fsm, const struct fsm_state *state)
{
	(void) fsm;

	assert(fsm != NULL);
	assert(state != NULL);

	return !!state->end;
}

int
fsm_hasend(const struct fsm *fsm)
{
	const struct fsm_state *s;

	assert(fsm != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		if (fsm_isend(fsm, s)) {
			return 1;
		}
	}

	return 0;
}

void
fsm_setstart(struct fsm *fsm, struct fsm_state *state)
{
	assert(fsm != NULL);
	assert(state != NULL);

	fsm->start = state;
}

struct fsm_state *
fsm_getstart(const struct fsm *fsm)
{
	assert(fsm != NULL);

	return fsm->start;
}

struct fsm_state *
fsm_getstatebyid(const struct fsm *fsm, unsigned int id)
{
	struct fsm_state *s;

	assert(fsm != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		if (s->id == id) {
			return s;
		}
	}

	return NULL;
}

unsigned int
fsm_getmaxid(const struct fsm *fsm)
{
	unsigned int max;
	struct fsm_state *s;

	assert(fsm != NULL);

	max = 0;
	for (s = fsm->sl; s != NULL; s = s->next) {
		if (s->id > max) {
			max = s->id;
		}
	}

	return max;
}

