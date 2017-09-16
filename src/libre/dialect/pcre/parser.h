/*
 * Automatically generated from the files:
 *	src/libre/dialect/pcre/parser.sid
 * and
 *	src/libre/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 448 "src/libre/parser.act"


	#include <re/re.h>

	typedef struct lex_state * lex_state;
	typedef struct act_state * act_state;

	typedef struct fsm * fsm;
	typedef struct fsm_state * t_fsm__state;
	typedef struct flags *flags;
	typedef struct re_err * err;

#line 26 "src/libre/dialect/pcre/parser.h"

/* BEGINNING OF FUNCTION DECLARATIONS */

extern void p_re__pcre(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
/* BEGINNING OF TRAILER */

#line 1346 "src/libre/parser.act"


#line 36 "src/libre/dialect/pcre/parser.h"

/* END OF FILE */
