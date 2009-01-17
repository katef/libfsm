/* $Id$ */

#ifndef FSM_OUT_H
#define FSM_OUT_H

struct fsm;

enum fsm_out {
	FSM_OUT_FSM,
	FSM_OUT_DOT,
	FSM_OUT_TABLE,
	FSM_OUT_C
};

/*
 * Output an FSM to the given file stream. The output is written in the format
 * specified. The available formats are:
 *
 *  FSM_OUT_FSM   - Native .fsm format, suitable for parsing by fsm_parse();
 *  FSM_OUT_DOT   - Graphviz Dot format, intended for rendering graphically;
 *  FSM_OUT_TABLE - A plaintext state transition matrix.
 *  FSM_OUT_C     - ISO C90 code.
 *
 * TODO: what to return?
 * TODO: explain constraints (e.g. single-character labels for C output)
 */
void
fsm_print(struct fsm *fsm, FILE *f, enum fsm_out format, int (*fputs)(const char *s, FILE *f));

#endif

