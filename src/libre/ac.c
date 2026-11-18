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

#include <fsm/fsm.h>
#include <adt/bitmap.h>
#include <adt/stateset.h>

#include "ac.h"

enum { POOL_BLOCK_SIZE = 256 };

struct trie_children {
	struct bm bm;
	struct trie_state *a[]; /* ordered by symbol */
};

struct trie_state {
	struct trie_children *children; /* may be NULL */
	struct trie_state *fail;
	struct state_set *endids; /* borrowing state_set for endids */

	fsm_state_t st;
	unsigned int index;
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
};

static struct trie_state *
get_child(const struct trie_state *ts, unsigned char sym)
{
	if (ts->children == NULL || !bm_get(&ts->children->bm, sym)) {
		return NULL;
	}

	return ts->children->a[bm_rank(&ts->children->bm, sym)];
}

/* sym must not already be present */
static int
add_child(struct trie_state *ts, unsigned char sym, struct trie_state *nx)
{
	struct trie_children *new;
	size_t count, rank;

	assert(get_child(ts, sym) == NULL);

	count = ts->children == NULL ? 0 : bm_count(&ts->children->bm);

	/*
	 * Normally I would double for each realloc, but in this case a trie
	 * typically has just one child per node. The whole point of the dense
	 * array + bitmap arrangement is to avoid allocating unused pointers
	 * to save memory overall.
	 */
	new = realloc(ts->children, sizeof *new + (count + 1) * sizeof *new->a);
	if (new == NULL) {
		return 0;
	}

	if (count == 0) {
		bm_clear(&new->bm);
	}

	/* shuffle along to make space */
	rank = bm_rank(&new->bm, sym);
	assert(count >= rank);
	memmove(&new->a[rank + 1], &new->a[rank], (count - rank) * sizeof *new->a);

	new->a[rank] = nx;
	bm_set(&new->bm, sym);

	ts->children = new;

	return 1;
}

static struct trie_state *
add_state(struct trie_graph *g)
{
	struct trie_state *st;

	if (g->pool == NULL || g->pool->n == POOL_BLOCK_SIZE) {
		struct trie_pool *p = malloc(sizeof *p);
		if (p == NULL) {
			return NULL;
		}

		p->states = malloc(POOL_BLOCK_SIZE * sizeof *p->states);
		if (p->states == NULL) {
			free(p);
			return NULL;
		}

		p->next = g->pool;
		p->n = 0;

		g->pool = p;
	}

	st = &g->pool->states[g->pool->n++];

	st->children = NULL;

	st->fail    = NULL;
	st->have_st = 0;

	st->index   = ++g->nstates;

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
			free(p->states[i].children);
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

	g->root = add_state(g);
	if (g->root == NULL) {
		free(g);
		return NULL;
	}

	return g;
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

		nx = get_child(st, (unsigned char) w[i]);
		if (nx == NULL) {
			nx = add_state(g);
			if (nx == NULL) {
				return NULL;
			}

			if (!add_child(st, (unsigned char) w[i], nx)) {
				return NULL;
			}
		}

		st = nx;
	}

	st->output = 1;
	if (g->depth < n) {
		g->depth = n;
	}

	if (endid != NULL) {
		state_set_add(&st->endids, NULL, (fsm_state_t) *endid);
	}

	return g;
}

int
trie_add_failure_edges(struct trie_graph *g)
{
	struct trie_state **q;
	size_t top,bot;
	int sym;

	q = malloc(g->nstates * sizeof *q);
	if (q == NULL) {
		return -1;
	}

	/* initialize queue and initial states */
	top = bot = 0;

	for (sym = 0; sym < 256; sym++) {
		struct trie_state *st;

		st = get_child(g->root, sym);
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

			nx = get_child(st, sym);
			if (nx == NULL) {
				continue;
			}

			assert(top < g->nstates);

			q[top++] = nx;

			fs = st->fail;
			assert(fs != NULL);

			while (get_child(fs, sym) == NULL && fs != g->root) {
				fs = fs->fail;
				assert(fs != NULL);
			}

			nx->fail = get_child(fs, sym);
			if (nx->fail == NULL) {
				assert(fs == g->root);
				nx->fail = fs;
			}

			nx->output = nx->output | fs->output;
		}
	}

	g->root->fail = g->root;

	free(q);

	return 0;
}

static struct trie_state *
find_next_state(struct trie_state *s, int sym)
{
	struct trie_state *nx, *fail;

	nx = get_child(s, sym);
	if (nx != NULL) {
		return nx;
	}

	fail = s->fail;
	if (fail == NULL) {
		return NULL;
	}


	nx = get_child(fail, sym);
	while (nx == NULL && fail->fail != fail) {
		fail = fail->fail;
		nx = get_child(fail, sym);
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
trie_to_fsm(struct fsm *fsm, struct trie_graph *g, int have_end, fsm_state_t end)
{
	fsm_state_t start;

	if (!trie_to_fsm_state(g->root, fsm, have_end, end, &start)) {
		return NULL;
	}

	fsm_setstart(fsm, start);

	return fsm;
}

static void
dump_state(struct trie_state *st, FILE *f, int indent, char* buf)
{
	int i;

	for (i = 0; i < 256; i++) {
		struct trie_state *nx;
		int sp;

		nx = get_child(st, i);
		if (nx != NULL) {
			fprintf(f, "%5d|", indent);
			for (sp=0; sp < indent; sp++) {
				fprintf(f, "  ");
			}
			fprintf(f, "<%5u> --[%3d %c]-- <%5u>\n", st->index, i, isprint(i) ? i : ' ', nx->index);

			buf[indent] = isprint(i) ? i : ' ';
			dump_state(nx,f,indent+1, buf);
		}
	}

	if (st->output) {
		buf[indent] = '\0';
		fprintf(f, "W:%s\n", buf);
	}
}

void
trie_dump(struct trie_graph *g, FILE *f)
{
	char *buf;

	buf = malloc(g->depth+1);
	if (buf == NULL) {
		perror("allocating buffer");
		return;
	}

	dump_state(g->root,f,0, buf);

	free(buf);
}

