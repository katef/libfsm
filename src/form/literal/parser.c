/*
 * Automatically generated from the files:
 *	form/literal/parser.sid
 * and
 *	libre/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 46 "libre/parser.act"


	#include <assert.h>
	#include <stdlib.h>
	#include <stdio.h>	/* XXX */
	#include <limits.h>

	#include <fsm/fsm.h>
	#include <re/re.h>

	#include "parser.h"
	#include "lexer.h"

	#include "../libre/internal.h"

	typedef char     t_char;
	typedef unsigned t_unsigned;
	typedef char *   t_grp;

	typedef struct fsm_state * t_fsm__state;

#line 35 "form/literal/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

extern void p_re__literal(re, lex_state, act_state);
static void p_list_Hof_Hliterals_C_Cliteral(re, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_list_Hof_Hliterals(re, lex_state, act_state, t_fsm__state, t_fsm__state);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

void
p_re__literal(re re, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_fsm__state ZIx;
		t_fsm__state ZIy;

		/* BEGINNING OF ACTION: make-states */
		{
#line 100 "libre/parser.act"

		assert(re != NULL);
		assert(re->fsm != NULL);
		/* TODO: assert re is empty */
		
		(ZIx) = fsm_addstate(re->fsm);
		if ((ZIx) == NULL) {
			goto ZL1;
		}

		fsm_setstart(re->fsm, (ZIx));

		(ZIy) = fsm_addstate(re->fsm);
		if ((ZIy) == NULL) {
			goto ZL1;
		}

		fsm_setend(re->fsm, (ZIy), 1);
	
#line 85 "form/literal/parser.c"
		}
		/* END OF ACTION: make-states */
		/* BEGINNING OF INLINE: 51 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					p_list_Hof_Hliterals (re, lex_state, act_state, ZIx, ZIy);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL3;
					}
				}
				break;
			default:
				{
					/* BEGINNING OF ACTION: add-epsilon */
					{
#line 199 "libre/parser.act"

		if (!fsm_addedge_epsilon(re->fsm, (ZIx), (ZIy))) {
			goto ZL3;
		}
	
#line 110 "form/literal/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			}
			goto ZL2;
		ZL3:;
			{
				/* BEGINNING OF ACTION: err-expected-items */
				{
#line 340 "libre/parser.act"

		act_state->err = RE_EXITEMS;
	
#line 125 "form/literal/parser.c"
				}
				/* END OF ACTION: err-expected-items */
			}
		ZL2:;
		}
		/* END OF INLINE: 51 */
		/* BEGINNING OF INLINE: 52 */
		{
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_EOF):
					break;
				default:
					goto ZL5;
				}
				ADVANCE_LEXER;
			}
			goto ZL4;
		ZL5:;
			{
				/* BEGINNING OF ACTION: err-expected-eof */
				{
#line 348 "libre/parser.act"

		act_state->err = RE_EXEOF;
	
#line 152 "form/literal/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL4:;
		}
		/* END OF INLINE: 52 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Hliterals_C_Cliteral(re re, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZIc;

		switch (CURRENT_TERMINAL) {
		case (TOK_CHAR):
			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 78 "libre/parser.act"

		ZIc = act_state->lex_tokval(lex_state);
	
#line 183 "form/literal/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: add-literal */
		{
#line 205 "libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		/* TODO: check c is legal? */

		if (!fsm_addedge_literal(re->fsm, (ZIx), (ZIy), (ZIc))) {
			goto ZL1;
		}
	
#line 204 "form/literal/parser.c"
		}
		/* END OF ACTION: add-literal */
		/* BEGINNING OF ACTION: count-1 */
		{
#line 325 "libre/parser.act"

		(void) (ZIx);
		(void) (ZIy);
	
#line 214 "form/literal/parser.c"
		}
		/* END OF ACTION: count-1 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Hliterals(re re, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_list_Hof_Hliterals:;
	{
		t_fsm__state ZIz;

		/* BEGINNING OF ACTION: add-concat */
		{
#line 192 "libre/parser.act"

		(ZIz) = fsm_addstate(re->fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 243 "form/literal/parser.c"
		}
		/* END OF ACTION: add-concat */
		p_list_Hof_Hliterals_C_Cliteral (re, lex_state, act_state, ZIx, ZIz);
		/* BEGINNING OF INLINE: 49 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					/* BEGINNING OF INLINE: list-of-literals */
					ZIx = ZIz;
					goto ZL2_list_Hof_Hliterals;
					/* END OF INLINE: list-of-literals */
				}
				/*UNREACHED*/
			default:
				{
					/* BEGINNING OF ACTION: add-epsilon */
					{
#line 199 "libre/parser.act"

		if (!fsm_addedge_epsilon(re->fsm, (ZIz), (ZIy))) {
			goto ZL1;
		}
	
#line 268 "form/literal/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 49 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

/* BEGINNING OF TRAILER */

#line 352 "libre/parser.act"


#line 291 "form/literal/parser.c"

/* END OF FILE */
