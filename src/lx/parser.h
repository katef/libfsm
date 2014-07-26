/*
 * Automatically generated from the files:
 *	parser.sid
 * and
 *	parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 86 "parser.act"


	#include <stdio.h>

	#include "ast.h"

	typedef struct lex_state * lex_state;
	typedef struct act_state * act_state;
	typedef struct ast *       ast;

	struct ast *lx_parse(FILE *f);

#line 26 "parser.h"

/* BEGINNING OF FUNCTION DECLARATIONS */

extern void p_lx(lex_state, act_state, ast *);
/* BEGINNING OF TRAILER */

#line 416 "parser.act"


#line 36 "parser.h"

/* END OF FILE */
