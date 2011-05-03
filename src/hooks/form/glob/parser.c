/*
 * Automatically generated from the files:
 *	parser.sid
 * and
 *	/Users/kate/svn/lx-combined2/src/lib/libre/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 46 "/Users/kate/svn/lx-combined2/src/lib/libre/parser.act"


	#include <assert.h>
	#include <stdlib.h>
	#include <stdio.h>	/* XXX */
	#include <limits.h>

	#include <fsm/fsm.h>
	#include <re/re.h>

	#include "parser.h"
	#include "lexer.h"

	#include "libre/internal.h"

	typedef char     t_char;
	typedef unsigned t_unsigned;
	typedef char *   t_grp;

	typedef struct fsm_state * t_fsm__state;

#line 35 "parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Hitems(re, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_list_Hof_Hitems_C_Citem(re, lex_state, act_state, t_fsm__state, t_fsm__state *);
static void p_list_Hof_Hitems_C_Cany(re, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_list_Hof_Hitems_C_Cwildcard(re, lex_state, act_state, t_fsm__state, t_fsm__state);
extern void p_re__glob(re, lex_state, act_state);
static void p_list_Hof_Hitems_C_Cliteral(re, lex_state, act_state, t_fsm__state, t_fsm__state);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Hitems(re re, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_list_Hof_Hitems:;
	{
		t_fsm__state ZIz;

		/* BEGINNING OF ACTION: add-concat */
		{
#line 188 "/Users/kate/svn/lx-combined2/src/lib/libre/parser.act"

		(ZIz) = fsm_addstate(re->fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 75 "parser.c"
		}
		/* END OF ACTION: add-concat */
		p_list_Hof_Hitems_C_Citem (re, lex_state, act_state, ZIx, &ZIz);
		/* BEGINNING OF INLINE: 55 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_QMARK): case (TOK_STAR): case (TOK_CHAR):
				{
					/* BEGINNING OF INLINE: list-of-items */
					ZIx = ZIz;
					goto ZL2_list_Hof_Hitems;
					/* END OF INLINE: list-of-items */
				}
				/*UNREACHED*/
			default:
				{
					/* BEGINNING OF ACTION: add-epsilon */
					{
#line 195 "/Users/kate/svn/lx-combined2/src/lib/libre/parser.act"

		if (!fsm_addedge_epsilon(re->fsm, (ZIz), (ZIy))) {
			goto ZL1;
		}
	
#line 100 "parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 55 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Hitems_C_Citem(re re, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state *ZIy)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_QMARK):
		{
			p_list_Hof_Hitems_C_Cany (re, lex_state, act_state, ZIx, *ZIy);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: count-1 */
			{
#line 336 "/Users/kate/svn/lx-combined2/src/lib/libre/parser.act"

		(void) (ZIx);
		(void) (*ZIy);
	
#line 136 "parser.c"
			}
			/* END OF ACTION: count-1 */
		}
		break;
	case (TOK_CHAR):
		{
			p_list_Hof_Hitems_C_Cliteral (re, lex_state, act_state, ZIx, *ZIy);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: count-1 */
			{
#line 336 "/Users/kate/svn/lx-combined2/src/lib/libre/parser.act"

		(void) (ZIx);
		(void) (*ZIy);
	
#line 155 "parser.c"
			}
			/* END OF ACTION: count-1 */
		}
		break;
	case (TOK_STAR):
		{
			p_list_Hof_Hitems_C_Cwildcard (re, lex_state, act_state, ZIx, *ZIy);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: count-0-or-many */
			{
#line 286 "/Users/kate/svn/lx-combined2/src/lib/libre/parser.act"

		if (!fsm_addedge_epsilon(re->fsm, (ZIx), (*ZIy))) {
			goto ZL1;
		}

		if (!fsm_addedge_epsilon(re->fsm, (*ZIy), (ZIx))) {
			goto ZL1;
		}

		/* isolation guard */
		/* TODO: centralise */
		{
			struct fsm_state *z;

			z = fsm_addstate(re->fsm);
			if (z == NULL) {
				goto ZL1;
			}

			if (!fsm_addedge_epsilon(re->fsm, (*ZIy), z)) {
				goto ZL1;
			}

			(*ZIy) = z;
		}
	
#line 196 "parser.c"
			}
			/* END OF ACTION: count-0-or-many */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	default:
		goto ZL1;
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Hitems_C_Cany(re re, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case (TOK_QMARK):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: add-any */
		{
#line 212 "/Users/kate/svn/lx-combined2/src/lib/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		if (!fsm_addedge_any(re->fsm, (ZIx), (ZIy))) {
			goto ZL1;
		}
	
#line 237 "parser.c"
		}
		/* END OF ACTION: add-any */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Hitems_C_Cwildcard(re re, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case (TOK_STAR):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: add-any */
		{
#line 212 "/Users/kate/svn/lx-combined2/src/lib/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		if (!fsm_addedge_any(re->fsm, (ZIx), (ZIy))) {
			goto ZL1;
		}
	
#line 272 "parser.c"
		}
		/* END OF ACTION: add-any */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

void
p_re__glob(re re, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_fsm__state ZIx;
		t_fsm__state ZIy;

		/* BEGINNING OF ACTION: make-states */
		{
#line 100 "/Users/kate/svn/lx-combined2/src/lib/libre/parser.act"

		assert(re != NULL);
		assert(re->fsm != NULL);
		/* TODO: assert re is empty */
		
		(ZIx) = fsm_getstart(re->fsm);
		assert((ZIx) != NULL);

		(ZIy) = fsm_addstate(re->fsm);
		if ((ZIy) == NULL) {
			goto ZL1;
		}

		re->end = (ZIy);
	
#line 310 "parser.c"
		}
		/* END OF ACTION: make-states */
		/* BEGINNING OF INLINE: 57 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_QMARK): case (TOK_STAR): case (TOK_CHAR):
				{
					p_list_Hof_Hitems (re, lex_state, act_state, ZIx, ZIy);
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
#line 195 "/Users/kate/svn/lx-combined2/src/lib/libre/parser.act"

		if (!fsm_addedge_epsilon(re->fsm, (ZIx), (ZIy))) {
			goto ZL3;
		}
	
#line 335 "parser.c"
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
#line 351 "/Users/kate/svn/lx-combined2/src/lib/libre/parser.act"

		act_state->err = RE_EXITEMS;
	
#line 350 "parser.c"
				}
				/* END OF ACTION: err-expected-items */
			}
		ZL2:;
		}
		/* END OF INLINE: 57 */
		/* BEGINNING OF INLINE: 58 */
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
#line 359 "/Users/kate/svn/lx-combined2/src/lib/libre/parser.act"

		act_state->err = RE_EXEOF;
	
#line 377 "parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL4:;
		}
		/* END OF INLINE: 58 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Hitems_C_Cliteral(re re, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
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
#line 78 "/Users/kate/svn/lx-combined2/src/lib/libre/parser.act"

		ZIc = act_state->lex_tokval(lex_state);
	
#line 408 "parser.c"
			}
			/* END OF EXTRACT: CHAR */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: add-literal */
		{
#line 201 "/Users/kate/svn/lx-combined2/src/lib/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		/* TODO: check c is legal? */

		if (!fsm_addedge_literal(re->fsm, (ZIx), (ZIy), (ZIc))) {
			goto ZL1;
		}
	
#line 429 "parser.c"
		}
		/* END OF ACTION: add-literal */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

/* BEGINNING OF TRAILER */

#line 363 "/Users/kate/svn/lx-combined2/src/lib/libre/parser.act"


#line 444 "parser.c"

/* END OF FILE */
