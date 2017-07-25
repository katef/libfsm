/*
 * Automatically generated from the files:
 *	src/libre/dialect/pcre/parser.sid
 * and
 *	src/libre/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 428 "src/libre/parser.act"


	#include <re/re.h>

	typedef struct lex_state * lex_state;
	typedef struct act_state * act_state;

	typedef struct fsm *  fsm;
	typedef struct flags *flags;
	typedef struct re_err * err;

#line 25 "src/libre/dialect/pcre/parser.h"

/* BEGINNING OF FUNCTION DECLARATIONS */

extern void p_re__pcre(fsm, flags, lex_state, act_state, err);
/* BEGINNING OF TRAILER */

#line 1259 "src/libre/parser.act"


#line 35 "src/libre/dialect/pcre/parser.h"

/* END OF FILE */
