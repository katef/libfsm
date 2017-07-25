#include <assert.h>
#include <stddef.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <adt/set.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/table.h>
#include <fsm/walk.h>

#include "./internal.h"

static int
count_state_things(const struct fsm *fsm, const struct fsm_state *st, void *opaque)
{
	struct fsm_table *tbl = opaque;

	tbl->nstates++;

	if (fsm_isend(fsm, st)) {
		tbl->nend++;
	}

	return 1;
}

static int
count_edge_things(const struct fsm *fsm,
    const struct fsm_state *src, unsigned int lbl, const struct fsm_state *dst, void *opaque)
{
	struct fsm_table *tbl = opaque;
	(void)fsm;
	(void)src;
	(void)lbl;
	(void)dst;
	tbl->nedges++;
	return 1;
}

static size_t
indexof(const struct fsm *fsm, const struct fsm_state *state)
{
	struct fsm_state *s;
	size_t i;

	assert(fsm != NULL);
	assert(state != NULL);

	for (s = fsm->sl, i = 0; s != NULL; s = s->next, i++) {
		if (s == state) {
			return i;
		}
	}

	assert(!"unreached");
	return 0;
}

struct builder {
	struct fsm_table *tbl;
	const struct fsm_state *src;
	size_t si;
	size_t end;
	size_t edge;
};

static int
build_state_info(const struct fsm *fsm, const struct fsm_state *st, void *opaque)
{
	struct builder *b = opaque;
	size_t i,si;
	int is_start, is_end;

	is_start = (st == fsm_getstart(fsm));
	is_end = fsm_isend(fsm,st);

        if (!is_start && !is_end) {
		return 1;
	}

	si = indexof(fsm,st);

	if (is_start) {
		assert(b->tbl->start == (size_t)-1);

		b->tbl->start = si;
	}

	if (is_end) {
		i = b->end++;
		assert(i < b->tbl->nend);

		b->tbl->endstates[i].state = si;
		b->tbl->endstates[i].opaque = fsm_getopaque((struct fsm *)fsm,st); /* XXX - why the const cast? */
	}

	return 1;
}

static int
build_edge_info(const struct fsm *fsm, const struct fsm_state *src, unsigned int lbl, const struct fsm_state *dst, void *opaque)
{
	struct builder *b = opaque;
	size_t si;
	size_t ind;

	if (src != b->src) {
		b->src = src;
		b->si = indexof(fsm, src);
	}
	si = b->si;

	ind = b->edge++;
        assert(ind < b->tbl->nedges);

	b->tbl->edges[ind].src = si;
	b->tbl->edges[ind].lbl = lbl;
	b->tbl->edges[ind].dst = indexof(fsm, dst);

	return 1;
}

struct fsm_table *
fsm_table(const struct fsm *fsm)
{
	struct builder b;
	struct fsm_table *tbl;

	assert(fsm != NULL);

	tbl = malloc(sizeof *tbl);
	if (tbl == NULL) {
		goto error;
	}

	/* Fills in the counts of the table so the caller can allocate memory */
	tbl->nstates = 0;
	tbl->nedges  = 0;
	tbl->nend    = 0;

	fsm_walk_states(fsm, tbl, count_state_things);
	fsm_walk_edges(fsm, tbl, count_edge_things);

	tbl->endstates = NULL;
	tbl->edges = NULL;

	tbl->endstates = malloc(tbl->nend * sizeof tbl->endstates[0]);
	if (tbl->endstates == NULL) {
		goto error;
	}

	tbl->edges = malloc(tbl->nedges * sizeof tbl->edges[0]);
	if (tbl->edges == NULL) {
		goto error;
	}

	tbl->start = (size_t)-1;
	b.tbl = tbl;
	b.end = 0;
	b.edge = 0;
	b.src = NULL;
	b.si = 0;
	fsm_walk_states(fsm, &b, build_state_info);
	fsm_walk_edges(fsm, &b, build_edge_info);

	return tbl;

error:
	if (tbl != NULL) {
		free(tbl->endstates);
		free(tbl->edges);
		free(tbl);
	}
	return NULL;
}

void
fsm_table_free(struct fsm_table *t)
{
	if (!t) { return; }
	free(t->endstates);
	free(t->edges);
	free(t);
}
