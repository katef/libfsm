/*
 * Automatically generated from the files:
 *	src/libre/form/glob/parser.sid
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

#line 34 "src/libre/form/glob/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Hatoms_C_Cliteral(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_list_Hof_Hatoms(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_list_Hof_Hatoms_C_Catom(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state *);
static void p_list_Hof_Hatoms_C_Cany(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_list_Hof_Hatoms_C_Cwildcard(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
extern void p_re__glob(fsm, cflags, lex_state, act_state);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Hatoms_C_Cliteral(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
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
	
#line 75 "src/libre/form/glob/parser.c"
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
	
#line 96 "src/libre/form/glob/parser.c"
		}
		/* END OF ACTION: add-literal */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Hatoms(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_list_Hof_Hatoms:;
	{
		t_fsm__state ZIz;

		/* BEGINNING OF ACTION: add-concat */
		{
#line 245 "src/libre/parser.act"

		(ZIz) = fsm_addstate(fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 125 "src/libre/form/glob/parser.c"
		}
		/* END OF ACTION: add-concat */
		p_list_Hof_Hatoms_C_Catom (fsm, cflags, lex_state, act_state, ZIx, &ZIz);
		/* BEGINNING OF INLINE: 62 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_QMARK): case (TOK_STAR): case (TOK_CHAR):
				{
					/* BEGINNING OF INLINE: list-of-atoms */
					ZIx = ZIz;
					goto ZL2_list_Hof_Hatoms;
					/* END OF INLINE: list-of-atoms */
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
	
#line 150 "src/libre/form/glob/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 62 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Hatoms_C_Catom(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state *ZIy)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_QMARK):
		{
			p_list_Hof_Hatoms_C_Cany (fsm, cflags, lex_state, act_state, ZIx, *ZIy);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: count-1 */
			{
#line 404 "src/libre/parser.act"

		(void) (ZIx);
		(void) (*ZIy);
	
#line 186 "src/libre/form/glob/parser.c"
			}
			/* END OF ACTION: count-1 */
		}
		break;
	case (TOK_CHAR):
		{
			p_list_Hof_Hatoms_C_Cliteral (fsm, cflags, lex_state, act_state, ZIx, *ZIy);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: count-1 */
			{
#line 404 "src/libre/parser.act"

		(void) (ZIx);
		(void) (*ZIy);
	
#line 205 "src/libre/form/glob/parser.c"
			}
			/* END OF ACTION: count-1 */
		}
		break;
	case (TOK_STAR):
		{
			p_list_Hof_Hatoms_C_Cwildcard (fsm, cflags, lex_state, act_state, ZIx, *ZIy);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: count-0-or-many */
			{
#line 354 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (*ZIy))) {
			goto ZL1;
		}

		if (!fsm_addedge_epsilon(fsm, (*ZIy), (ZIx))) {
			goto ZL1;
		}

		/* isolation guard */
		/* TODO: centralise */
		{
			struct fsm_state *z;

			z = fsm_addstate(fsm);
			if (z == NULL) {
				goto ZL1;
			}

			if (!fsm_addedge_epsilon(fsm, (*ZIy), z)) {
				goto ZL1;
			}

			(*ZIy) = z;
		}
	
#line 246 "src/libre/form/glob/parser.c"
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
p_list_Hof_Hatoms_C_Cany(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
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
#line 280 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		if (!fsm_addedge_any(fsm, (ZIx), (ZIy))) {
			goto ZL1;
		}
	
#line 287 "src/libre/form/glob/parser.c"
		}
		/* END OF ACTION: add-any */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Hatoms_C_Cwildcard(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
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
#line 280 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		if (!fsm_addedge_any(fsm, (ZIx), (ZIy))) {
			goto ZL1;
		}
	
#line 322 "src/libre/form/glob/parser.c"
		}
		/* END OF ACTION: add-any */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

void
p_re__glob(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state)
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
	
#line 359 "src/libre/form/glob/parser.c"
		}
		/* END OF ACTION: make-states */
		/* BEGINNING OF INLINE: 64 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_QMARK): case (TOK_STAR): case (TOK_CHAR):
				{
					p_list_Hof_Hatoms (fsm, cflags, lex_state, act_state, ZIx, ZIy);
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
	
#line 384 "src/libre/form/glob/parser.c"
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
	
#line 399 "src/libre/form/glob/parser.c"
				}
				/* END OF ACTION: err-expected-atoms */
			}
		ZL2:;
		}
		/* END OF INLINE: 64 */
		/* BEGINNING OF INLINE: 65 */
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
	
#line 426 "src/libre/form/glob/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL4:;
		}
		/* END OF INLINE: 65 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

/* BEGINNING OF TRAILER */

#line 431 "src/libre/parser.act"


#line 445 "src/libre/form/glob/parser.c"

/* END OF FILE */
