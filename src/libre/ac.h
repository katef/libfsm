/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef AC_H
#define AC_H

#include "fsm/fsm.h"

struct fsm;
struct fsm_state;

struct trie_graph;

struct trie_graph *
trie_create(void);

void
trie_free(struct trie_graph *g);

struct trie_graph *
trie_add_word(struct trie_graph *g, const char *w, size_t n,
	const fsm_end_id_t *endid);

int
trie_add_failure_edges(struct trie_graph *g);

struct fsm *
trie_to_fsm(struct fsm *fsm, struct trie_graph *g,
	int have_end, fsm_state_t end);

void
trie_dump(struct trie_graph *g, FILE *f);

#endif

