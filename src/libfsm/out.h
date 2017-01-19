/* $Id$ */

#ifndef FSM_INTERNAL_OUT_H
#define FSM_INTERNAL_OUT_H

#include <stdio.h>

#include <fsm/out.h>

/* XXX: perhaps to move under fsm_outoptions alongside .fragment */
int
fsm_out_cfrag(const struct fsm *fsm, FILE *f, const struct fsm_outoptions *options,
	int (*leaf)(FILE *, const struct fsm *, const struct fsm_state *, void *),
	void *opaque);

void
fsm_out_stateenum(FILE *f, const struct fsm *fsm, struct fsm_state *sl);

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

