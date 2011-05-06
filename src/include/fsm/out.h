/* $Id$ */

#ifndef FSM_OUT_H
#define FSM_OUT_H

#include <stdio.h>

struct fsm;

enum fsm_out {
	FSM_OUT_FSM,
	FSM_OUT_DOT,
	FSM_OUT_DOTFRAG,
	FSM_OUT_TABLE,
	FSM_OUT_C,
	FSM_OUT_CFRAG
};

/*
 * Output an FSM to the given file stream. The output is written in the format
 * specified. The available formats are:
 *
 *  FSM_OUT_FSM   - fsm(5) .fsm format, suitable for parsing by fsm(1);
 *  FSM_OUT_DOT   - Graphviz Dot format, intended for rendering graphically;
 *  FSM_OUT_TABLE - A plaintext state transition matrix.
 *  FSM_OUT_C     - ISO C90 code.
 *
 * The -FRAG equivalents are fragments of code, rather than complete programs.
 * These are intended to be suitable for including into custom code.
 *
 * TODO: what to return?
 * TODO: explain constraints (e.g. single-character labels for C output)
 */
void
fsm_print(struct fsm *fsm, FILE *f, enum fsm_out format);

#endif

