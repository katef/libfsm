/* $Id$ */

#include <assert.h>
#include <stdlib.h>

#include <fsm/fsm.h>
#include <fsm/colour.h>

#include "internal.h"
#include "set.h"

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

void
fsm_move(struct fsm *dst, struct fsm *src)
{
	assert(src != NULL);
	assert(dst != NULL);

	free_contents(dst);

	dst->sl      = src->sl;
	dst->start   = src->start;
	dst->options = src->options;

	dst->colour_hooks = src->colour_hooks;

	free(src);
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

