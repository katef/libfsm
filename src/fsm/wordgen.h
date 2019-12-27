/*
 * Copyright 2019 Shannon F. Stewman
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef WORDGEN_H
#define WORDGEN_H

#include <stdio.h>

#include "xoroshiro256starstar.h"

struct fsm;

struct dfa_table {
	unsigned *offsets;
	unsigned char *syms;
	unsigned *dests;

	unsigned char *isend;

	unsigned nstates;
	unsigned nedges;
	unsigned start;
};

int dfa_table_initialize(struct dfa_table *table, const struct fsm *fsm);
void dfa_table_finalize(struct dfa_table *table);

struct dfa_wordgen_params {
	unsigned minlen;
	unsigned maxlen;

	float prob_stop;
	float eps_weight;
};

char **dfa_generate_words(const struct dfa_table *table, const struct dfa_wordgen_params *params, struct prng_state *prng, unsigned num_words);
int dfa_generate_words_to_file(const struct dfa_table *table, const struct dfa_wordgen_params *params, struct prng_state *prng, unsigned num_words, FILE *f);

void free_dfa_generated_words(char **w);

/* convenience wrappers */
char **fsm_generate_words(const struct fsm *fsm, const struct dfa_wordgen_params *params, struct prng_state *prng, unsigned num_words);
int fsm_generate_words_to_file(const struct fsm *fsm, const struct dfa_wordgen_params *params, struct prng_state *prng, unsigned num_words, FILE *f);

#endif /* WORDGEN_H */

