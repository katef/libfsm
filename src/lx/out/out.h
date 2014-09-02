/* $Id$ */

#ifndef OUT_H
#define OUT_H

#include <stdio.h>

struct ast;

struct prefix {
	const char *api;
	const char *tok;
	const char *lx;
};

extern struct prefix prefix;

void
lx_out_c(const struct ast *ast, FILE *f);

void
lx_out_h(const struct ast *ast, FILE *f);

void
lx_out_dot(const struct ast *ast, FILE *f);

#endif

