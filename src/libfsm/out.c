/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <stdio.h>
#include <assert.h>

#include <fsm/fsm.h>
#include <fsm/out.h>

#include "out.h"

void
fsm_print(struct fsm *fsm, FILE *f, enum fsm_out format)
{
	void (*out)(const struct fsm *fsm, FILE *f) = NULL;

	assert(fsm != NULL);
	assert(f != NULL);

	switch (format) {
	case FSM_OUT_API:  out = fsm_out_api;  break;
	case FSM_OUT_C:    out = fsm_out_c;    break;
	case FSM_OUT_CSV:  out = fsm_out_csv;  break;
	case FSM_OUT_DOT:  out = fsm_out_dot;  break;
	case FSM_OUT_FSM:  out = fsm_out_fsm;  break;
	case FSM_OUT_JSON: out = fsm_out_json; break;
	}

	assert(out != NULL);

	out(fsm, f);
}

