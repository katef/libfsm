/* $Id$ */

#ifndef OUT_DOT_H
#define OUT_DOT_H

#include <stdio.h>

void out_fsm(FILE *f, const struct fsm *fsm);

void out_dot(FILE *f, const struct fsm *fsm);

void out_table(FILE *f, const struct fsm *fsm);

#endif

