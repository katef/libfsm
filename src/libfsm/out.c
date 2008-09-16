/* $Id$ */

#include <stdio.h>
#include <assert.h>

#include <fsm/out.h>

#include "out/out.h"

void
fsm_print(struct fsm *fsm, FILE *f, enum fsm_out format)
{
	void (*out)(const struct fsm *fsm, FILE *f) = NULL;

	assert(f);
	assert(fsm);

	switch (format) {
	case FSM_OUT_FSM:   out = out_fsm;   break;
	case FSM_OUT_DOT:   out = out_dot;   break;
	case FSM_OUT_TABLE: out = out_table; break;
	case FSM_OUT_C:     out = out_c;     break;
	}

	assert(out != NULL);

	out(fsm, f);
}

