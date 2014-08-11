/* $Id$ */

#ifndef FSM_EXEC_H
#define FSM_EXEC_H

struct fsm;
struct fsm_state;

/*
 * Execute an FSM reading input from the user-specified callback fsm_getc().
 * fsm_getc() is passed the opaque pointer given, and is expected to return
 * either an unsigned character cast to int, or EOF to indicate the end of
 * input.
 *
 * Returns the accepting state on a successful parse (i.e. where execution
 * ends in an state set by fsm_setend). Returns NULL for unexpected input,
 * premature EOF, or ending in a state not marked as an end state.
 *
 * The returned accepting state is intended to facillitate lookup of
 * its state opaque value previously set by fsm_setopaque() (not to be
 * confused with the opaque pointer passed for the fsm_getc callback function).
 *
 * The given FSM is expected to be a DFA.
 */
struct fsm_state *
fsm_exec(const struct fsm *fsm, int (*fsm_getc)(void *opaque), void *opaque);

/*
 * Callbacks which may be passed to fsm_exec(). These are conveniences for
 * common situations; they could equally well be user-defined.
 *
 *  fsm_sgetc - To read from a string. Pass the address of a pointer to the
 *              first element of a string:
 *
 *                const char *s = "abc";
 *                fsm_exec(fsm, fsm_sgetc, &s);
 *
 *              Where s will be incremented to point to each character in turn.
 *
 * TODO: add fsm_fgetc
 */
int
fsm_sgetc(void *opaque);

#endif

