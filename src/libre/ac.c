#include "ac.h"

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#include <ctype.h>
#include <string.h>

#include <fsm/fsm.h>

enum { POOL_BLOCK_SIZE = 256 };

struct trie_pool {
	struct trie_pool *next;
	struct trie_state *states;
	size_t n;
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

	memset(st->children,0,sizeof st->children);

	st->fail   = NULL;
	st->st     = NULL;

	st->index  = ++g->nstates;

	st->output = 0;

	return st;
}

static void
cleanup_pool(struct trie_graph *g)
{
	struct trie_pool *p;
	while (g->pool != NULL) {
		p = g->pool;
		g->pool = p->next;

		free(p->states);
		free(p);
	}
}

void
trie_free(struct trie_graph *g)
{
	if (g != NULL) {
		cleanup_pool(g);
		free(g);
	}
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

	g->root = newstate(g);
	if (g->root == NULL) {
		free(g);
		return NULL;
	}

	return g;
}

struct trie_graph *
trie_add_word(struct trie_graph *g, const char *w, size_t n)
{
	size_t i;
	struct trie_state *st;

	assert(g != NULL);

	st = g->root;

	assert(st != NULL);

	for (i=0; i < n; i++) {
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

			nx->fail = fs;
			nx->output = nx->output | fs->output;
		}
	}

	free(q);

	return 0;
}

static struct fsm_state *
trie_to_fsm_state(struct trie_state *ts, struct fsm *fsm)
{
	struct fsm_state *st;
	int sym;

	if (ts->st != NULL) {
		return ts->st;
	}

	st = fsm_addstate(fsm);
	if (st == NULL) {
		return NULL;
	}

	ts->st = st;

	for (sym=0; sym < 256; sym++) {
		struct trie_state *nx;
		struct fsm_state *dst;

		nx = (ts->children[sym] != NULL) ? ts->children[sym] : ts->fail;
		if (nx == NULL) {
			continue;
		}

		dst = trie_to_fsm_state(nx, fsm);
		if (dst == NULL) {
			return NULL;
		}

		if (!fsm_addedge_literal(fsm, st, dst, sym)) {
			return NULL;
		}
	}

	if (ts->output) {
		fsm_setend(fsm, st, 1);
	}

	return st;
}

struct fsm *
trie_to_fsm(struct trie_graph *g, const struct fsm_options* opts)
{
	struct fsm *fsm;
	struct fsm_state *st;

	fsm = fsm_new(opts);
	st = trie_to_fsm_state(g->root, fsm);
	fsm_setstart(fsm,st);

	return fsm;
}

static void
dump_state(struct trie_state *st, FILE *f, int indent, char* buf)
{
	int i;
	for (i=0; i < 256; i++) {
		int sp;

		if (st->children[i] != NULL) {
			fprintf(f, "%5d|", indent);
			for (sp=0; sp < indent; sp++) {
				fprintf(f, "  ");
			}
			fprintf(f, "<%5u> --[%3d %c]-- <%5u>\n", st->index, i, isprint(i) ? i : ' ', st->children[i]->index);

			buf[indent] = isprint(i) ? i : ' ';
			dump_state(st->children[i],f,indent+1, buf);
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
}


