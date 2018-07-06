/*
 * Copyright 2018 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef PRINT_EDGE_H
#define PRINT_EDGE_H

#include "libfsm/internal.h" /* XXX */

struct fsm_options;

typedef int (escpute)(FILE *f, const struct fsm_options *opt,
	enum fsm_edge_type symbol);

escpute dot_escpute_html;

#endif

