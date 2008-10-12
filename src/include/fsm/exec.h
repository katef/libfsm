/* $Id$ */

#ifndef FSM_EXEC_H
#define FSM_EXEC_H

struct fsm;

/*
 * Execute an FSM reading input from the user-specified callback fsm_getc().
 * fsm_getc() is passed the opaque pointer given, and is expected to return
 * either an unsigned character cast to int, or EOF to indicate the end of
 * input.
 *
 * Returns true on a successful parse, and false for unexpected input, or
 * premature EOF. If true, the value of the end state as set by fsm_setend()
 * is returned.
 *
 * The given FSM is expected to be a DFA.
 */
int
fsm_exec(const struct fsm *fsm, int (*fsm_getc)(void *opaque), void *opaque);

#endif

