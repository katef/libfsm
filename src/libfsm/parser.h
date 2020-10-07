/*
 * Automatically generated from the files:
 *	src/libfsm/parser.sid
 * and
 *	src/libfsm/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 150 "src/libfsm/parser.act"


	typedef struct lex_state * lex_state;
	typedef struct act_state * act_state;
	typedef struct fsm *       fsm;

	struct fsm *
	fsm_parse(FILE *f, const struct fsm_options *opt);

#line 23 "src/libfsm/parser.h"

/* BEGINNING OF FUNCTION DECLARATIONS */

extern void p_fsm(fsm, lex_state, act_state);
/* BEGINNING OF TRAILER */

#line 436 "src/libfsm/parser.act"

#line 32 "src/libfsm/parser.h"

/* END OF FILE */
