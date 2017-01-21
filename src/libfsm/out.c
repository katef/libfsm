/* $Id$ */

#include <stdio.h>
#include <assert.h>

#include <fsm/fsm.h>
#include <fsm/out.h>

#include "out.h"

void
fsm_print(struct fsm *fsm, FILE *f, enum fsm_out format,
	const struct fsm_outoptions *options)
{
	static const struct fsm_outoptions defaults;

	void (*out)(const struct fsm *fsm, FILE *f, const struct fsm_outoptions *options) = NULL;

	assert(fsm != NULL);
	assert(f != NULL);

	switch (format) {
	case FSM_OUT_C:    out = fsm_out_c;    break;
	case FSM_OUT_CSV:  out = fsm_out_csv;  break;
	case FSM_OUT_DOT:  out = fsm_out_dot;  break;
	case FSM_OUT_FSM:  out = fsm_out_fsm;  break;
	case FSM_OUT_JSON: out = fsm_out_json; break;
	}

	assert(out != NULL);

	if (options == NULL) {
		options = &defaults;
	}

	out(fsm, f, options);
}

