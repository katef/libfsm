/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <ctype.h>

#include <print/esc.h>
#include <fsm/options.h>

#include <fsm/fsm.h>
#include <adt/stateset.h>

#include "ac.h"

enum { POOL_BLOCK_SIZE = 256 };

struct trie_state {
	struct trie_state *children[256];
	struct trie_state *fail;
	/* use a state set as an endid set */
	struct state_set *endids;

	fsm_state_t st;
	unsigned int output:1;
	unsigned int have_st:1;
};

struct trie_pool {
	struct trie_pool *next;
	struct trie_state *states;
	size_t n;
};

struct trie_graph {
	struct trie_state *root;
	struct trie_pool *pool;
	size_t nstates;
	size_t depth;
	unsigned int has_endids:1;
};

static struct trie_state *
newstate(struct trie_graph *g)
{
	struct trie_state *st;

	if (g->pool == NULL || g->pool->n == POOL_BLOCK_SIZE) {
		struct trie_pool *p = malloc(sizeof *p);
		if (p == NULL) {
			return NULL;
		}

		p->states = calloc(POOL_BLOCK_SIZE, sizeof p->states[0]);
		if (p->states == NULL) {
			free(p);
			return NULL;
		}

		p->next = g->pool;
		p->n = 0;

		g->pool = p;
	}

	st = &g->pool->states[g->pool->n++];
	g->nstates++;

	memset(st->children,0,sizeof st->children);

	st->fail    = NULL;
	st->have_st = 0;

	st->output  = 0;
	st->endids  = NULL;

	return st;
}

static void
cleanup_pool(struct trie_graph *g)
{
	struct trie_pool *p;

	while (g->pool != NULL) {
		p = g->pool;
		g->pool = p->next;

		for (size_t i = 0; i < p->n; i++) {
			state_set_free(p->states[i].endids);
		}

		free(p->states);
		free(p);
	}
}

void
trie_free(struct trie_graph *g)
{
	if (g == NULL) {
		return;
	}

	cleanup_pool(g);
	free(g);
}

struct trie_graph *
trie_create(void)
{
	struct trie_graph *g;

	g = malloc(sizeof *g);
	if (g == NULL) {
		return NULL;
	}

	g->pool = NULL;
	g->nstates = 0;
	g->depth = 0;
	g->has_endids = 0;

	g->root = newstate(g);
	if (g->root == NULL) {
		free(g);
		return NULL;
	}

	return g;
}

bool
trie_has_endids(const struct trie_graph *g)
{
	assert(g != NULL);

	return g->has_endids;
}

struct trie_graph *
trie_add_word(struct trie_graph *g, const char *w, size_t n, const fsm_end_id_t *endid)
{
	struct trie_state *st;
	size_t i;

	assert(g != NULL);

	st = g->root;

	assert(st != NULL);

	for (i = 0; i < n; i++) {
		struct trie_state *nx;
		int idx = (unsigned char)w[i];
		nx = st->children[idx];

		if (nx == NULL) {
			nx = newstate(g);
			if (nx == NULL) {
				return NULL;
			}

			st->children[idx] = nx;
		}

		st = nx;
	}

	st->output = 1;
	if (g->depth < n) {
		g->depth = n;
	}

	if (endid != NULL) {
		g->has_endids = 1;
		if (!state_set_add(&st->endids, NULL, (fsm_state_t)*endid)) {
			return NULL;
		}
	}

	return g;
}

int
trie_add_failure_edges(struct trie_graph *g) {
	struct trie_state **q;
	size_t top,bot;
	int sym;

	q = malloc(g->nstates * sizeof q[0]);
	if (q == NULL) {
		return -1;
	}

	/* initialize queue and initial states */
	top = bot = 0;

	for (sym = 0; sym < 256; sym++) {
		struct trie_state *st;

		st = g->root->children[sym];
		if (st == NULL) {
			continue;
		}

		st->fail = g->root;
		q[top++] = st;
	}

	/* proceed in bfs through states */
	while (bot < top) {
		struct trie_state *st;
		int sym;

		st = q[bot++];

		for (sym=0; sym < 256; sym++) {
			struct trie_state *fs, *nx;

			nx = st->children[sym];
			if (nx == NULL) {
				continue;
			}

			assert(top < g->nstates);

			q[top++] = nx;

			fs = st->fail;
			assert(fs != NULL);

			while (fs->children[sym] == NULL && fs != g->root) {
				fs = fs->fail;
				assert(fs != NULL);
			}

			nx->fail = fs->children[sym];
			if (nx->fail == NULL) {
				assert(fs == g->root);
				nx->fail = fs;
			}

			nx->output |= nx->fail->output;

			{
				struct state_iter it; /* borrowing state_set for endids */
				fsm_state_t endid;

				for (state_set_reset(nx->fail->endids, &it); state_set_next(&it, &endid); ) {
					/*
					 * TODO: Do we care about duplicates? maybe this is where we
					 * implement AMBIG_? not sure
					 * TODO: need to pass in alloc functions, none of ac.c does this
					 */
					if (!state_set_add(&nx->endids, NULL, (fsm_state_t) endid)) {
						return -1;
					}
				}
			}
		}
	}

	g->root->fail = g->root;

	free(q);

	return 0;
}

static struct trie_state *
find_next_state(const struct trie_state *s, int sym)
{
	struct trie_state *nx, *fail;

	nx = s->children[sym];
	if (nx != NULL) {
		return nx;
	}

	fail = s->fail;
	if (fail == NULL) {
		return NULL;
	}


	nx = fail->children[sym];
	while (nx == NULL && fail->fail != fail) {
		fail = fail->fail;
		nx = fail->children[sym];
	}

	if (nx == NULL) {
		assert(fail->fail == fail);

		/*
		 * failure state is the root, which
		 * has implicit edges back to itself
		 */
		return fail;
	}

	return nx;
}

static int
trie_to_fsm_state(struct trie_state *ts, struct fsm *fsm,
	int have_end, fsm_state_t single_end,
	fsm_state_t *q)
{
	fsm_state_t st;
	int sym;

	assert(fsm != NULL);
	assert(q != NULL);

	if (ts->output && have_end && state_set_empty(ts->endids)) {
		*q = single_end;
		return 1;
	}

	if (ts->have_st) {
		*q = ts->st;
		return 1;
	}

	/* TODO: bulk add states in advance */
	if (!fsm_addstate(fsm, &st)) {
		return 0;
	}

	ts->st = st;
	ts->have_st = 1;

	for (sym = 0; sym < 256; sym++) {
		struct trie_state *nx;

		nx = find_next_state(ts,sym);
		if (nx != NULL) {
			fsm_state_t dst;

			if (!trie_to_fsm_state(nx, fsm, have_end, single_end, &dst)) {
				return 0;
			}

			if (!fsm_addedge_literal(fsm, st, dst, (char)sym)) {
				return 0;
			}
		}
	}

	if (ts->output) {
		fsm_setend(fsm, st, 1);

		struct state_iter si;
		fsm_state_t state;
		state_set_reset(ts->endids, &si);
		while (state_set_next(&si, &state)) {
			if (!fsm_endid_set(fsm, st, (fsm_end_id_t) state)) {
				return 0;
			}
		}
	}

	*q = st;
	return 1;
}

struct fsm *
trie_to_fsm(struct fsm *fsm, const struct trie_graph *g, int have_end, fsm_state_t end)
{
	fsm_state_t start;

	if (!trie_to_fsm_state(g->root, fsm, have_end, end, &start)) {
		return NULL;
	}

	fsm_setstart(fsm, start);

	return fsm;
}

struct dump_state {
	const struct trie_state *st;
	unsigned rank;
};

static void
dump_rank(struct dump_state *ds, const struct trie_state *st, unsigned int rank, FILE *f, size_t *index)
{
	unsigned i;

	assert(ds != NULL);
	assert(st != NULL);
	assert(index != NULL);

	ds[*index].st   = st;
	ds[*index].rank = rank;
	(*index)++;

	for (i = 0; i < 256; i++) {
		if (st->children[i] != NULL) {
			dump_rank(ds, st->children[i], rank + 1, f, index);
		}
	}
}

static int
ds_rank_cmp(const void *a, const void *b)
{
	const struct dump_state *da = a;
	const struct dump_state *db = b;

	/* these can only happen when there are bugs,
	 * but this code is for debugging... */
	if (da == NULL) { return +1; }
	if (db == NULL) { return -1; }

	if (da->rank < db->rank) { return -1; }
	if (da->rank > db->rank) { return +1; }

	return 0;
}

/*
 * Picking up a state index from the allocator pool means we don't need to
 * store the index inside struct trie_state. This way we can match the
 * numbering from the academic paper (using the same numbering helps for
 * debugging!) and keep the memory footprint svelte.
 *
 * The downside of course is that this search is slow. Which is fine for
 * our purposes debugging.
 *
 * This is based on the assumption that the allocations are tracked in
 * the order that they're made.
 */
static unsigned
state_index(const struct trie_pool *pool, const struct trie_state *state)
{
	unsigned index;
	size_t i;

	assert(pool != NULL);

	index = 0;

	do {
		for (i = 0; i < pool->n; i++) {
			if (&pool->states[i] == state) {
				return index;
			}

			index++;
		}
	} while (pool = pool->next, pool != NULL);

	assert(!"unreached");
	abort();
}

void
trie_dump(const struct trie_graph *g, FILE *f)
{
	struct dump_state *ds;

	assert(g != NULL);
	assert(g->root != NULL);
	assert(f != NULL);

	fprintf(f, "digraph G {\n");
	fprintf(f, "\tnode [ ordering = out, shape = box, style = rounded, width = 0.3, label = \"\" ];\n");
	fprintf(f, "\tedge [ dir = none, style = invis ];\n");
	fprintf(f, "\tranksep = 0;\n");
	fprintf(f, "\tTBbalance = min;\n");
	fprintf(f, "\troot = n%p;\n", (const void *) g->root);
	fprintf(f, "\n");

	ds = malloc(g->nstates * sizeof *ds);
	if (ds == NULL) {
		perror("malloc");
		abort();
	}

	{
		size_t count = 0;
		dump_rank(ds, g->root, 0, f, &count);
		assert(count == g->nstates);
	}

	qsort(ds, g->nstates, sizeof *ds, ds_rank_cmp);

	{
		size_t i;
		unsigned rank;

		rank = 0;

		fprintf(f, "\tsubgraph { rank = same; ");
		for (i = 0; i < g->nstates; i++) {
			if (ds[i].rank != rank) {
				fprintf(f, "}\n");
				fprintf(f, "\tsubgraph { rank = same; ");
				rank = ds[i].rank;
			}

			fprintf(f, "n%p", (const void *) ds[i].st);
			if (i < g->nstates && ds[i + 1].rank == rank) {
				/* cross-links for horizontal ordering within a rank */
				fprintf(f, " -> ");
			} else {
				fprintf(f, "; ");
			}
		}
		fprintf(f, "}\n");
		fprintf(f, "\n");
	}

	/* nodes */
	{
		unsigned i;

		for (i = 0; i < g->nstates; i++) {
			fprintf(f, "\tn%p [ ",
				(const void *) ds[i].st);

			if (ds[i].st->output) {
				fprintf(f, "peripheries = 2, ");
			}

			fprintf(f, "label = \n");
			fprintf(f, "\t\t<<TABLE BORDER='0' CELLPADDING='2' CELLSPACING='0'>\n");

			fprintf(f, "\t\t\t<TR>");
			fprintf(f, "<TD COLSPAN='2' ALIGN='LEFT'>S%u</TD>", state_index(g->pool, ds[i].st));
			fprintf(f, "</TR>");

			fprintf(f, "\t\t\t<TR>");
			fprintf(f, "<TD COLSPAN='2' ALIGN='LEFT'>fail</TD><TD ALIGN='LEFT'>");
			if (ds[i].st->fail == NULL) {
				fprintf(f, "-");
			} else {
				fprintf(f, "S%u", state_index(g->pool, ds[i].st->fail));
			}
			fprintf(f, "</TD>");
			fprintf(f, "</TR>\n");

			fprintf(f, "\t\t\t<TR>");
			fprintf(f, "<TD COLSPAN='2' ALIGN='LEFT'>endids</TD>");
			fprintf(f, "<TD ALIGN='LEFT'>");
			if (!ds[i].st->output) {
				fprintf(f, "-");
			} else if (state_set_count(ds[i].st->endids) == 0) {
				fprintf(f, "∅");
			} else {
				struct state_iter it;
				fsm_state_t endid;
				int j;

				for (j = 0, state_set_reset(ds[i].st->endids, &it); state_set_next(&it, &endid); j = 1) {
					if (j != 0) {
						fprintf(f, ", ");
					}
					fprintf(f, "#%u", endid);
				}
			}
			fprintf(f, "</TD>");
			fprintf(f, "</TR>\n");

			fprintf(f, "</TABLE>> ];\n");
		}

		fprintf(f, "\n");
	}

	/* trie edges */
	{
		unsigned i;

		for (i = 0; i < g->nstates; i++) {
			int sym;

			for (sym = 0; sym < 256; sym++) {
				if (ds[i].st->children[sym] != NULL) {
					static const struct fsm_options opt;
					fprintf(f, "\tn%p -> n%p [ style = solid; label = <",
						(const void *) ds[i].st, (const void *) ds[i].st->children[sym]);
					dot_escputc_html(f, &opt, (char) sym);
					fprintf(f, "> ];\n");
				}
			}
		}

		fprintf(f, "\n");
	}

	/* fail edges */
	{
		unsigned i;

		for (i = 0; i < g->nstates; i++) {
			if (ds[i].st->fail) {
				fprintf(f, "\tn%p -> n%p [ dir = back, style = dashed ];\n",
					(const void *) ds[i].st->fail, (const void *) ds[i].st);
			}
		}
	}

	free(ds);

	fprintf(f, "}\n");
}

