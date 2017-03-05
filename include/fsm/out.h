/* $Id$ */

#ifndef FSM_OUT_H
#define FSM_OUT_H

#include <stdio.h>

struct fsm;

enum fsm_out {
	FSM_OUT_API,
	FSM_OUT_C,
	FSM_OUT_CSV,
	FSM_OUT_DOT,
	FSM_OUT_FSM,
	FSM_OUT_JSON
};

enum fsm_io {
	FSM_IO_GETC,
	FSM_IO_STR,
	FSM_IO_PAIR
};

/*
 * Output an FSM to the given file stream. The output is written in the format
 * specified. The available formats are:
 *
 *  FSM_OUT_API  - C code which calls the fsm(3) API
 *  FSM_OUT_C    - ISO C90 code
 *  FSM_OUT_CSV  - A comma-separated state transition matrix
 *  FSM_OUT_DOT  - Graphviz Dot format, intended for rendering graphically
 *  FSM_OUT_FSM  - fsm(5) .fsm format, suitable for parsing by fsm(1)
 *  FSM_OUT_JSON - JavaScript Object Notation
 *
 * The output options may be NULL, indicating to use defaults.
 *
 * TODO: what to return?
 * TODO: explain constraints (e.g. single-character labels for C output)
 */
void
fsm_print(struct fsm *fsm, FILE *f, enum fsm_out format);

#endif

