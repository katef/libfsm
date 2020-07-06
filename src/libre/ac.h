/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef AC_H
#define AC_H

struct fsm;
struct fsm_state;
struct fsm_options;

struct trie_graph;

struct trie_graph *
trie_create(void);

void
trie_free(struct trie_graph *g);

struct trie_graph *
trie_add_word(struct trie_graph *g, const char *w, size_t n);

int
trie_add_failure_edges(struct trie_graph *g);

struct fsm *
trie_to_fsm(struct fsm *fsm, fsm_state_t *start, struct trie_graph *g,
	int have_end, fsm_state_t end);

void
trie_dump(struct trie_graph *g, FILE *f);

#endif

