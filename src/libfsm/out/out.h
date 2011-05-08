/* $Id$ */

#ifndef OUT_H
#define OUT_H

#include <stdio.h>

#include <fsm/out.h>

void out_fsm(const struct fsm *fsm, FILE *f, const struct fsm_outoptions *options);

void out_dot(const struct fsm *fsm, FILE *f, const struct fsm_outoptions *options);

void out_table(const struct fsm *fsm, FILE *f, const struct fsm_outoptions *options);

void out_c(const struct fsm *fsm, FILE *f, const struct fsm_outoptions *options);

#endif

