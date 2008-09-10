/* $Id$ */

#ifndef OUT_DOT_H
#define OUT_DOT_H

#include <stdio.h>

struct fsm_options;

void out_fsm(FILE *f, const struct fsm_options *options,
	struct state_list *sl, struct label_list *ll, struct fsm_state *start);

void out_dot(FILE *f, const struct fsm_options *options,
	struct state_list *sl, struct label_list *ll, struct fsm_state *start);

void out_table(FILE *f, const struct fsm_options *options,
	struct state_list *sl, struct label_list *ll, struct fsm_state *start);

#endif

