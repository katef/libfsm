/* $Id$ */

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <fsm/fsm.h>

#include "internal.h"

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
	new->ll = NULL;
	new->start = NULL;
	new->options = default_options;

	return new;
}

void
fsm_free(struct fsm *fsm)
{
	assert(fsm != NULL);

	/* TODO */
}

struct fsm_state *
fsm_addstate(struct fsm *fsm, unsigned int id, int end)
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

		p->state.id    = id;
		p->state.edges = NULL;
		p->state.end   = end;

		p->next = fsm->sl;
		fsm->sl = p;
	}

	return &p->state;
}

struct fsm_edge *
fsm_addedge(struct fsm *fsm, struct fsm_state *from, struct fsm_state *to,
	const char *label)
{
	struct label_list *p;
	struct fsm_edge   *e;

	/* Find an existing label */
	for (p = fsm->ll; p; p = p->next) {
		if (label == NULL && p->label == NULL) {
			break;
		}

		if (label == NULL || p->label == NULL) {
			continue;
		}

		if (0 == strcmp(p->label, label)) {
			break;
		}
	}

	/* Otherwise, create a new one */
	if (p == NULL) {
		p = malloc(sizeof *p);
		if (p == NULL) {
			return NULL;
		}

		p->label = label;

		p->next = fsm->ll;
		fsm->ll = p;
	}

	e = malloc(sizeof *e);
	if (e == NULL) {
		return NULL;
	}

	e->label = p;
	e->state = to;

	e->next  = from->edges;
	from->edges = e;

	return e;
}

void
fsm_setoptions(struct fsm *fsm, struct fsm_options *options)
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

	return state->end;
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

	for (p = fsm->sl; p; p = p->next) {
		if (p->state.id == id) {
			return &p->state;
		}
	}

	return NULL;
}

