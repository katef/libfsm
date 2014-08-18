/*
 * Automatically generated from the files:
 *	src/libre/form/literal/parser.sid
 * and
 *	src/libre/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 38 "src/libre/parser.act"


	#include <assert.h>
	#include <stdlib.h>
	#include <limits.h>

	#include <fsm/fsm.h>

	#include <re/re.h>

	#include "parser.h"
	#include "lexer.h"

	typedef char     t_char;
	typedef unsigned t_unsigned;
	typedef char *   t_grp;
	typedef unsigned t_pred; /* TODO */

	typedef struct fsm_state * t_fsm__state;

#line 34 "src/libre/form/literal/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

extern void p_re__literal(fsm, cflags, lex_state, act_state);
static void p_list_Hof_Hliterals_C_Cliteral(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_list_Hof_Hliterals(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

void
p_re__literal(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_fsm__state ZIx;
		t_fsm__state ZIy;

		/* BEGINNING OF ACTION: make-states */
		{
#line 157 "src/libre/parser.act"

		assert(fsm != NULL);
		/* TODO: assert fsm is empty */

		(ZIx) = fsm_getstart(fsm);
		assert((ZIx) != NULL);

		(ZIy) = fsm_addstate(fsm);
		if ((ZIy) == NULL) {
			goto ZL1;
		}

		fsm_setend(fsm, (ZIy), 1);
	
#line 79 "src/libre/form/literal/parser.c"
		}
		/* END OF ACTION: make-states */
		/* BEGINNING OF INLINE: 58 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					p_list_Hof_Hliterals (fsm, cflags, lex_state, act_state, ZIx, ZIy);
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
#line 252 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIy))) {
			goto ZL3;
		}
	
#line 104 "src/libre/form/literal/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			}
			goto ZL2;
		ZL3:;
			{
				/* BEGINNING OF ACTION: err-expected-atoms */
				{
#line 419 "src/libre/parser.act"

		act_state->e = RE_EXATOMS;
	
#line 119 "src/libre/form/literal/parser.c"
				}
				/* END OF ACTION: err-expected-atoms */
			}
		ZL2:;
		}
		/* END OF INLINE: 58 */
		/* BEGINNING OF INLINE: 59 */
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
#line 427 "src/libre/parser.act"

		act_state->e = RE_EXEOF;
	
#line 146 "src/libre/form/literal/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL4:;
		}
		/* END OF INLINE: 59 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Hliterals_C_Cliteral(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
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
#line 111 "src/libre/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIc = lex_state->buf.a[0];
	
#line 180 "src/libre/form/literal/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: add-literal */
		{
#line 269 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		/* TODO: check c is legal? */

		if (!fsm_addedge_literal(fsm, (ZIx), (ZIy), (ZIc))) {
			goto ZL1;
		}
	
#line 201 "src/libre/form/literal/parser.c"
		}
		/* END OF ACTION: add-literal */
		/* BEGINNING OF ACTION: count-1 */
		{
#line 404 "src/libre/parser.act"

		(void) (ZIx);
		(void) (ZIy);
	
#line 211 "src/libre/form/literal/parser.c"
		}
		/* END OF ACTION: count-1 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Hliterals(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_list_Hof_Hliterals:;
	{
		t_fsm__state ZIz;

		/* BEGINNING OF ACTION: add-concat */
		{
#line 245 "src/libre/parser.act"

		(ZIz) = fsm_addstate(fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 240 "src/libre/form/literal/parser.c"
		}
		/* END OF ACTION: add-concat */
		p_list_Hof_Hliterals_C_Cliteral (fsm, cflags, lex_state, act_state, ZIx, ZIz);
		/* BEGINNING OF INLINE: 56 */
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
#line 252 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIz), (ZIy))) {
			goto ZL1;
		}
	
#line 265 "src/libre/form/literal/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 56 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

/* BEGINNING OF TRAILER */

#line 431 "src/libre/parser.act"


#line 288 "src/libre/form/literal/parser.c"

/* END OF FILE */
