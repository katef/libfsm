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

static void
state_remove(struct fsm_state **head, struct fsm_state *state)
{
	struct fsm_state **s;

	assert(head != NULL);
	assert(state != NULL);

	for (s = head; *s != NULL; s = &(*s)->next) {
		if (*s == state) {
			struct fsm_state *next;

			next = (*s)->next;
			free(*s);
			*s = next;
			break;
		}
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
	new->start = NULL;

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
	dst->start = src->start;

	free(src);
}

struct fsm_state *
fsm_addstate(struct fsm *fsm)
{
	struct fsm_state *new;
	int i;

	assert(fsm != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->end = 0;

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		new->edges[i].sl = NULL;
	}

	new->opaque = NULL;

	new->next = fsm->sl;
	fsm->sl = new;

	return new;
}

void
fsm_removestate(struct fsm *fsm, struct fsm_state *state)
{
	struct fsm_state *s;
	int i;

	assert(fsm != NULL);
	assert(state != NULL);

	for (s = fsm->sl; s != NULL; s = s->next) {
		for (i = 0; i <= FSM_EDGE_MAX; i++) {
			set_remove(&s->edges[i].sl, state);
		}
	}

	for (i = 0; i <= FSM_EDGE_MAX; i++) {
		set_free(state->edges[i].sl);
	}

	if (fsm->start == state) {
		fsm->start = NULL;
	}

	state_remove(&fsm->sl, state);
}

void
fsm_setend(struct fsm *fsm, struct fsm_state *state, int end)
{
	(void) fsm;

	assert(fsm != NULL);
	assert(state != NULL);

	state->end = !!end;
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

	fsm->start = state;
}

struct fsm_state *
fsm_getstart(const struct fsm *fsm)
{
	assert(fsm != NULL);

	return fsm->start;
}

