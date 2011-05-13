/* $Id$ */

#ifndef OUT_H
#define OUT_H

#include <stdio.h>

struct ast;

void out_c(const struct ast *ast, FILE *f);

void out_h(const struct ast *ast, FILE *f);

void out_dot(const struct ast *ast, FILE *f);

#endif

