/* $Id$ */

#include <assert.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include "internal.h"

static unsigned int inventid(const struct fsm *fsm) {
	assert(fsm != NULL);

	return fsm_getmaxid(fsm) + 1;
}

static void free_contents(struct fsm *fsm) {
	void *next;
	struct state_set *s;
	struct state_set *e;

	assert(fsm != NULL);

	for (s = fsm->sl; s; s = next) {
		assert(s->state != NULL);

		for (e = s->state->el; e; e = next) {
			next = e->next;
			free(e);
		}

		free(s->state);

		next = s->next;
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

	new->sl = NULL;
	new->start = NULL;
	new->options = default_options;

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
	struct state_set *s;
	struct fsm *new;

	new = fsm_new();
	if (new == NULL) {
		return NULL;
	}

	/* recreate states */
	for (s = fsm->sl; s; s = s->next) {
		struct fsm_state *state;

		state = fsm_addstate(new, s->state->id);
		if (state == NULL) {
			fsm_free(new);
			return NULL;
		}

		state->opaque = s->state->opaque;

		if (fsm_isend(fsm, s->state)) {
			fsm_setend(new, state, 1);
		}
	}

	/* recreate edges */
	for (s = fsm->sl; s; s = s->next) {
		struct state_set *e;
		struct fsm_state *from;
		int i;

		from = fsm_getstatebyid(new, s->state->id);

		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			struct fsm_state *to;

			assert(s->state->edges[i] != NULL);

			to = fsm_getstatebyid(new, s->state->edges[i]->id);

			assert(from != NULL);
			assert(to   != NULL);

			if (!fsm_addedge_literal(new, from, to, i)) {
				fsm_free(new);
				return NULL;
			}
		}

		for (e = s->state->el; e; e = e->next) {
			struct fsm_state *to;

			to = fsm_getstatebyid(new, e->state->id);

			assert(from != NULL);
			assert(to   != NULL);

			if (!fsm_addedge_epsilon(new, from, to)) {
				fsm_free(new);
				return NULL;
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
	struct state_set **sl;
	struct state_set *p;

	assert(dst != NULL);
	assert(src != NULL);

	/*
	 * Splice over lists from src to dst.
	 */

	for (sl = &dst->sl; *sl; sl = &(*sl)->next) {
		/* nothing */
	}
	*sl = src->sl;


	/*
	 * Renumber incoming states so as not to clash over existing ones.
	 */
	for (p = src->sl; p; p = p->next) {
		p->state->id = 0;
	}
	for (p = src->sl; p; p = p->next) {
		p->state->id = inventid(dst);
	}


	/*
	 * Add epsilon transition from dst's start state to src's start state.
	 */
	if (src->start != NULL) {
		if (dst->start == NULL) {
			dst->start = src->start;
		} else if (!fsm_addedge_epsilon(dst, dst->start, src->start)) {
			/* TODO: this leaves dst in a questionable state */
			return 0;
		}
	} else {
		/* TODO: src has no start state. rethink during API refactor */
	}


	free(src);

	return 1;
}

struct fsm_state *
fsm_addstate(struct fsm *fsm, unsigned int id)
{
	struct state_set *p;

	assert(fsm != NULL);

	/* Find an existing state */
	for (p = fsm->sl; p; p = p->next) {
		if (p->state->id == id) {
			break;
		}
	}

	/* Otherwise, create a new one */
	if (p == NULL) {
		struct fsm_state *new;
		int i;

		new = malloc(sizeof *new);
		if (new == NULL) {
			return NULL;
		}

		p = malloc(sizeof *p);
		if (p == NULL) {
			free(new);
			return NULL;
		}

		new->id     = id == 0 ? inventid(fsm) : id;
		new->opaque = NULL;
		new->end    = 0;
		new->el     = NULL;

		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			new->edges[i] = NULL;
		}

		p->state = new;

		p->next  = fsm->sl;
		fsm->sl  = p;
	}

	return p->state;
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
	const struct state_set *s;

	assert(fsm != NULL);

	for (s = fsm->sl; s; s = s->next) {
		if (fsm_isend(fsm, s->state)) {
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
	struct state_set *p;

	assert(fsm != NULL);
	assert(id != 0);

	for (p = fsm->sl; p; p = p->next) {
		if (p->state->id == id) {
			return p->state;
		}
	}

	return NULL;
}

unsigned int
fsm_getmaxid(const struct fsm *fsm)
{
	unsigned int max;
	struct state_set *p;

	assert(fsm != NULL);

	max = 0;
	for (p = fsm->sl; p; p = p->next) {
		if (p->state->id > max) {
			max = p->state->id;
		}
	}

	return max;
}

void
fsm_setopaque(struct fsm *fsm, struct fsm_state *state, void *opaque)
{
	assert(fsm != NULL);
	assert(state != NULL);

	state->opaque = opaque;
}

void *
fsm_getopaque(struct fsm *fsm, struct fsm_state *state)
{
	assert(fsm != NULL);
	assert(state != NULL);

	return state->opaque;
}

