#ifndef AC_H
#define AC_H

#define MODULE_NAME AC

#include <stdio.h>

struct fsm;
struct fsm_state;
struct fsm_options;

struct trie_state {
	struct trie_state *children[256];
	struct trie_state *fail;
	struct fsm_state *st;
	unsigned int index;
	unsigned int output:1;
};

struct trie_pool;

struct trie_graph {
	struct trie_state *root;
	struct trie_pool *pool;
	size_t nstates;
	size_t depth;
};

struct trie_graph *
trie_create(void);

void
trie_free(struct trie_graph *g);

struct trie_graph *
trie_add_word(struct trie_graph *g, const char *w, size_t n);

int
trie_add_failure_edges(struct trie_graph *g);

struct fsm *
trie_to_fsm(struct trie_graph *g, const struct fsm_options* opts, int single_match);

void
trie_dump(struct trie_graph *g, FILE *f);

#undef MODULE_NAME

#endif /* AC_H */

