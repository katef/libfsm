/*
 * Automatically generated from the files:
 *	src/lx/parser.sid
 * and
 *	src/lx/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 140 "src/lx/parser.act"


	#include <stdio.h>

	#include "ast.h"

	typedef struct lex_state * lex_state;
	typedef struct act_state * act_state;
	typedef struct ast *       ast;

	struct ast *lx_parse(FILE *f, const struct fsm_options *opt);

#line 26 "src/lx/parser.h"

/* BEGINNING OF FUNCTION DECLARATIONS */

extern void p_lx(lex_state, act_state, ast *);
/* BEGINNING OF TRAILER */

#line 888 "src/lx/parser.act"


#line 36 "src/lx/parser.h"

/* END OF FILE */
