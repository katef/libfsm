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

struct fsm *
fsm_from_table(const struct fsm_table *tbl, const struct fsm_options *opts)
{
	struct fsm *fsm;
	struct fsm_state **states;
	size_t i,n;

	assert(tbl != NULL);

	fsm = NULL;
	states = NULL;

	states = malloc(tbl->nstates * sizeof states[0]);
	if (states == NULL) {
		goto error;
	}

	fsm = fsm_new(opts);
	if (fsm == NULL) {
		goto error;
	}

	n = tbl->nstates;
	for (i=0; i < n; i++) {
		states[i] = NULL;
	}

	for (i=0; i < n; i++) {
		struct fsm_state *st;
		if (st = fsm_addstate(fsm), st == NULL) {
			goto error;
		}
		states[i] = st;
	}

	if (tbl->start != (size_t)-1) {
		assert(tbl->start < tbl->nstates);
		fsm_setstart(fsm, states[tbl->start]);
	}

	n = tbl->nend;
	for (i=0; i < n; i++) {
		size_t ind;

		ind = tbl->endstates[i].state;
		assert(ind <= tbl->nstates);

		fsm_setend(fsm, states[ind], 1);
	}

	n = tbl->nedges;
	for (i=0; i < n; i++) {
		struct fsm_state *src, *dst;
		size_t si,di;

		si = tbl->edges[i].src;
		di = tbl->edges[i].dst;

		assert(si > 0 && si <= tbl->nstates);
		assert(di > 0 && di <= tbl->nstates);

		src = states[si-1];
		dst = states[di-1];

		fsm_addedge(src, dst, (enum fsm_edge_type)tbl->edges[i].lbl);
	}

	free(states);
	return fsm;

error:
	if (states != NULL) {
		for (i=0; i < tbl->nstates; i++) {
			free(states[i]);
		}
		free(states);
	}

	if (fsm != NULL) {
		fsm_free(fsm);
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

struct fsm_table *
fsm_read_table(FILE *f)
{
	unsigned long nstates, start, nend, nedges;
	size_t lineno = 0;
	size_t i;
	int nf, nchar;
	char line[1024];
	char *r;

	struct fsm_table *tbl = NULL;

	/* XXX - avoid using fixed buffer in the future */
	lineno++;
	if (r = fgets(line,sizeof line, f), r == NULL) {
		goto eof_or_error;
	}

	if (line[strlen(line)-1] != '\n') {
		goto line_too_long;
	}

	nstates = 0;
	start   = 0;
	nend    = 0;
	nedges  = 0;
	nf = sscanf(line, "%lu %lu %lu %lu\n%n", &nstates, &start, &nend, &nedges, &nchar);
	if (nf != 4 || line[nchar] != '\0') {
		goto bad_line;
	}

	if (start > nstates) {
		goto bad_state;
	}

	tbl = malloc(sizeof *tbl);
	if (tbl == NULL) {
		goto error;
	}

	tbl->endstates = NULL;
	tbl->edges = NULL;

	tbl->endstates = malloc(nend * sizeof tbl->endstates[0]);
	if (tbl->endstates == NULL) {
		goto error;
	}

	tbl->edges = malloc(nedges * sizeof tbl->edges[0]);
	if (tbl->edges == NULL) {
		goto error;
	}
	tbl->nstates = nstates;
	tbl->start   = start-1;
	tbl->nend    = nend;
	tbl->nedges  = nedges;

	for (i=0; i < nend; i++) {
		unsigned long st;

		lineno++;
		if (r = fgets(line,sizeof line, f), r == NULL) {
			goto eof_or_error;
		}

		if (line[strlen(line)-1] != '\n') {
			goto line_too_long;
		}

		/* XXX - need to check for more than one thing on the line */
		if (sscanf(line, "%lu\n%n", &st, &nchar) != 1) {
			goto bad_line;
		}
		if (line[nchar] != '\0') {
			goto bad_line;
		}
		if ((st < 1) || (st > nstates)) {
			goto bad_state;
		}

		tbl->endstates[i].state = st-1;
		tbl->endstates[i].opaque = NULL;
	}

	for (i=0; i < nedges; i++) {
		unsigned long src, dst;
		unsigned int lbl;

		lineno++;
		if (r = fgets(line, sizeof line, f), r == NULL) {
			goto eof_or_error;
		}

		if (line[strlen(line)-1] != '\n') {
			goto line_too_long;
		}

		/* XXX - need to check for more than one thing on the line */
		if (sscanf(line, "%lu %u %lu\n%n", &src, &lbl, &dst, &nchar) != 3) {
			goto bad_line;
		}

		if (line[nchar] != '\0') {
			goto bad_line;
		}

		if ((src < 1) || (src > nstates) || (dst < 1) || (dst > nstates)) {
			goto bad_state;
		}

		tbl->edges[i].src = src;
		tbl->edges[i].lbl = lbl;
		tbl->edges[i].dst = dst;
	}

	return tbl;

eof_or_error:
	if (feof(f)) {
		errno = EPERM;
	}
	goto error;

line_too_long:
bad_line:
bad_state:
	errno = EPERM;
	goto error;

error:
	if (tbl != NULL) {
		free(tbl->endstates);
		free(tbl->edges);
		free(tbl);
	}
	return NULL;
}



