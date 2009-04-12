/* $Id$ */

#ifndef FSM_PARSE_H
#define FSM_PARSE_H

struct fsm;

/*
 * Parse .fsm input from the given file stream. The fsm passed is expected to
 * have been created by fsm_new(); it should not have any existing states.
 * TODO: Have this create an fsm, instead
 */
struct fsm *
fsm_parse(struct fsm *fsm, FILE *f);

#endif

