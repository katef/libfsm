/* $Id$ */

#include <assert.h>
#include <stdlib.h>
#include <limits.h>

#include <fsm/fsm.h>

#include "internal.h"

static unsigned int inventid(const struct fsm *fsm) {
	unsigned int max;
	struct state_list *p;

	max = 0;
	for (p = fsm->sl; p; p = p->next) {
		if (p->state.id > max) {
			max = p->state.id;
		}
	}

	return max + 1;
}

static void free_contents(struct fsm *fsm) {
	void *next;
	struct state_list *s;
	struct trans_list *t;
	struct fsm_edge *e;
#ifndef NDEBUG
	static const struct fsm_state state_defaults;
#endif

	assert(fsm != NULL);

	for (s = fsm->sl; s; s = next) {
		struct fsm_edge *e_next;

		next = s->next;

		for (e = s->state.edges; e; e = e_next) {
			e_next = e->next;

#ifndef NDEBUG
			e->trans = NULL;
			e->state = NULL;
			e->next  = NULL;
#endif
			free(e);
		}

#ifndef NDEBUG
		s->state = state_defaults;
		s->next = NULL;
#endif

		free(s);
	}

	for (t = fsm->tl; t; t = next) {
		next = t->next;

		if (t->type == FSM_EDGE_LABEL) {
			free(t->u.label);

#ifndef NDEBUG
			t->u.label = NULL;
#endif
		}

		free(t);
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
	new->tl = NULL;
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
	struct state_list *s;
	struct fsm_edge *e;
	struct fsm *new;

	new = fsm_new();
	if (new == NULL) {
		return NULL;
	}

	/* recreate states */
	for (s = fsm->sl; s; s = s->next) {
		struct fsm_state *state;

		state = fsm_addstate(new, s->state.id);
		if (state == NULL) {
			fsm_free(new);
			return NULL;
		}

		if (fsm_isend(fsm, &s->state)) {
			fsm_setend(new, state, 1);
		}
	}

	/* recreate edges */
	for (s = fsm->sl; s; s = s->next) {
		for (e = s->state.edges; e; e = e->next) {
			struct fsm_state *from;
			struct fsm_state *to;

			from = fsm_getstatebyid(new, s->state.id);
			to   = fsm_getstatebyid(new, e->state->id);

			assert(from != NULL);
			assert(to   != NULL);

			if (fsm_addedge_copy(new, from, to, e) == NULL) {
				fsm_free(new);
				return NULL;
			}
		}
	}

	new->start = fsm_getstatebyid(new, fsm->start->id);
	if (new->start == NULL) {
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
	dst->tl      = src->tl;
	dst->start   = src->start;
	dst->options = src->options;

#ifndef NDEBUG
	src->sl    = NULL;
	src->tl    = NULL;
	src->start = NULL;
#endif

	free(src);
}

struct fsm_state *
fsm_addstate(struct fsm *fsm, unsigned int id)
{
	struct state_list *p;

	assert(fsm != NULL);

	/* Find an existing state */
	for (p = fsm->sl; p; p = p->next) {
		if (p->state.id == id) {
			break;
		}
	}

	/* Otherwise, create a new one */
	if (p == NULL) {
		p = malloc(sizeof *p);
		if (p == NULL) {
			return NULL;
		}

		p->state.id    = id == 0 ? inventid(fsm) : id;
		p->state.edges = NULL;
		p->state.end   = 0;

		p->next = fsm->sl;
		fsm->sl = p;
	}

	return &p->state;
}

void
fsm_setoptions(struct fsm *fsm, const struct fsm_options *options)
{
	assert(fsm != NULL);
	assert(options != NULL);

	fsm->options = *options;
}

void
fsm_setend(struct fsm *fsm, struct fsm_state *state, unsigned int end)
{
	(void) fsm;

	assert(fsm != NULL);
	assert(state != NULL);
	assert(end <= INT_MAX);

	state->end = end;
}

int
fsm_isend(const struct fsm *fsm, const struct fsm_state *state)
{
	(void) fsm;

	assert(fsm != NULL);
	assert(state != NULL);

	return state->end;
}

int
fsm_hasend(const struct fsm *fsm)
{
	const struct state_list *s;

	assert(fsm != NULL);

	for (s = fsm->sl; s; s = s->next) {
		if (fsm_isend(fsm, &s->state)) {
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
	struct state_list *p;

	assert(fsm != NULL);
	assert(id != 0);

	for (p = fsm->sl; p; p = p->next) {
		if (p->state.id == id) {
			return &p->state;
		}
	}

	return NULL;
}

