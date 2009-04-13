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
 * Returns true on a successful parse ending in an end state as set by
 * fsm_setend(), and false for unexpected input, premature EOF, or ending in
 * a state not marked as an end state.
 *
 * If true, the state id is returned; this is intended to facillitate lookup of
 * its state opaque value previously set by fsm_setopaque() (not to be confused
 * with the opaque pointer passed for the fsm_getc callback function).
 *
 * The given FSM is expected to be a DFA.
 */
int
fsm_exec(const struct fsm *fsm, int (*fsm_getc)(void *opaque), void *opaque);

#endif

