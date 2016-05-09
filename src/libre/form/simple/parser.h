/*
 * Automatically generated from the files:
 *	src/libre/form/simple/parser.sid
 * and
 *	src/libre/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 192 "src/libre/parser.act"


	#include <re/re.h>

	typedef struct lex_state * lex_state;
	typedef struct act_state * act_state;

	typedef struct fsm *  fsm;
	typedef enum re_flags flags;
	typedef struct re_err * err;

#line 25 "src/libre/form/simple/parser.h"

/* BEGINNING OF FUNCTION DECLARATIONS */

extern void p_re__simple(fsm, flags, lex_state, act_state, err);
extern void p_group_Hsimple(fsm, flags, lex_state, act_state, err);
/* BEGINNING OF TRAILER */

#line 821 "src/libre/parser.act"


#line 36 "src/libre/form/simple/parser.h"

/* END OF FILE */
