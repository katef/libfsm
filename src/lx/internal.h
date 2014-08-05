/* $Id$ */

#ifndef LX_INTERNAL_H
#define LX_INTERNAL_H

#include <stdio.h>

struct ast;

enum lx_out {
	LX_OUT_C,
	LX_OUT_H,
	LX_OUT_DOT,
	LX_OUT_TEST
};

void
lx_print(struct ast *ast, FILE *f, enum lx_out format);

#endif

