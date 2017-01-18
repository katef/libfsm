/* $Id$ */

#ifndef OUT_H
#define OUT_H

#include <stdio.h>

#include <fsm/out.h>

/* XXX: perhaps to move under fsm_outoptions alongside .fragment */
int
fsm_out_cfrag(const struct fsm *fsm, FILE *f, const struct fsm_outoptions *options,
    void (*singlecase)(FILE *, const struct fsm *, struct fsm_state *, void *opaque),
    void *opaque);

void
fsm_out_c(const struct fsm *fsm, FILE *f,
	const struct fsm_outoptions *options);

void
fsm_out_csv(const struct fsm *fsm, FILE *f,
	const struct fsm_outoptions *options);

void
fsm_out_dot(const struct fsm *fsm, FILE *f,
	const struct fsm_outoptions *options);

void
fsm_out_fsm(const struct fsm *fsm, FILE *f,
	const struct fsm_outoptions *options);

void
fsm_out_json(const struct fsm *fsm, FILE *f,
	const struct fsm_outoptions *options);

#endif

