/*
 * Automatically generated from the files:
 *	src/libre/form/glob/parser.sid
 * and
 *	src/libre/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 37 "src/libre/parser.act"


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

	typedef struct fsm_state * t_fsm__state;

#line 33 "src/libre/form/glob/parser.c"

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Hitems(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_list_Hof_Hitems_C_Citem(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state *);
static void p_list_Hof_Hitems_C_Cany(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_list_Hof_Hitems_C_Cwildcard(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
extern void p_re__glob(fsm, cflags, lex_state, act_state);
static void p_list_Hof_Hitems_C_Cliteral(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Hitems(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == 16) {
		return;
	}
ZL2_list_Hof_Hitems:;
	{
		t_fsm__state ZIz;

		/* BEGINNING OF ACTION: add-concat */
		{
#line 187 "src/libre/parser.act"

		(ZIz) = fsm_addstate(fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 68 "src/libre/form/glob/parser.c"
		}
		/* END OF ACTION: add-concat */
		p_list_Hof_Hitems_C_Citem (fsm, cflags, lex_state, act_state, ZIx, &ZIz);
		/* BEGINNING OF INLINE: 55 */
		{
			switch (CURRENT_TERMINAL) {
			case 2: case 3: case 13:
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
#line 194 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIz), (ZIy))) {
			goto ZL1;
		}
	
#line 93 "src/libre/form/glob/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			case 16:
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 55 */
	}
	return;
ZL1:;
	SAVE_LEXER (16);
	return;
}

static void
p_list_Hof_Hitems_C_Citem(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state *ZIy)
{
	switch (CURRENT_TERMINAL) {
	case 2:
		{
			p_list_Hof_Hitems_C_Cany (fsm, cflags, lex_state, act_state, ZIx, *ZIy);
			if ((CURRENT_TERMINAL) == 16) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: count-1 */
			{
#line 335 "src/libre/parser.act"

		(void) (ZIx);
		(void) (*ZIy);
	
#line 129 "src/libre/form/glob/parser.c"
			}
			/* END OF ACTION: count-1 */
		}
		break;
	case 13:
		{
			p_list_Hof_Hitems_C_Cliteral (fsm, cflags, lex_state, act_state, ZIx, *ZIy);
			if ((CURRENT_TERMINAL) == 16) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: count-1 */
			{
#line 335 "src/libre/parser.act"

		(void) (ZIx);
		(void) (*ZIy);
	
#line 148 "src/libre/form/glob/parser.c"
			}
			/* END OF ACTION: count-1 */
		}
		break;
	case 3:
		{
			p_list_Hof_Hitems_C_Cwildcard (fsm, cflags, lex_state, act_state, ZIx, *ZIy);
			if ((CURRENT_TERMINAL) == 16) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: count-0-or-many */
			{
#line 285 "src/libre/parser.act"

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
	
#line 189 "src/libre/form/glob/parser.c"
			}
			/* END OF ACTION: count-0-or-many */
		}
		break;
	case 16:
		return;
	default:
		goto ZL1;
	}
	return;
ZL1:;
	SAVE_LEXER (16);
	return;
}

static void
p_list_Hof_Hitems_C_Cany(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == 16) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case 2:
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: add-any */
		{
#line 211 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		if (!fsm_addedge_any(fsm, (ZIx), (ZIy))) {
			goto ZL1;
		}
	
#line 230 "src/libre/form/glob/parser.c"
		}
		/* END OF ACTION: add-any */
	}
	return;
ZL1:;
	SAVE_LEXER (16);
	return;
}

static void
p_list_Hof_Hitems_C_Cwildcard(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == 16) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case 3:
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: add-any */
		{
#line 211 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		if (!fsm_addedge_any(fsm, (ZIx), (ZIy))) {
			goto ZL1;
		}
	
#line 265 "src/libre/form/glob/parser.c"
		}
		/* END OF ACTION: add-any */
	}
	return;
ZL1:;
	SAVE_LEXER (16);
	return;
}

void
p_re__glob(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == 16) {
		return;
	}
	{
		t_fsm__state ZIx;
		t_fsm__state ZIy;

		/* BEGINNING OF ACTION: make-states */
		{
#line 99 "src/libre/parser.act"

		assert(fsm != NULL);
		/* TODO: assert fsm is empty */

		(ZIx) = fsm_getstart(fsm);
		assert((ZIx) != NULL);

		(ZIy) = fsm_addstate(fsm);
		if ((ZIy) == NULL) {
			goto ZL1;
		}

		fsm_setend(fsm, (ZIy), 1);
	
#line 302 "src/libre/form/glob/parser.c"
		}
		/* END OF ACTION: make-states */
		/* BEGINNING OF INLINE: 57 */
		{
			switch (CURRENT_TERMINAL) {
			case 2: case 3: case 13:
				{
					p_list_Hof_Hitems (fsm, cflags, lex_state, act_state, ZIx, ZIy);
					if ((CURRENT_TERMINAL) == 16) {
						RESTORE_LEXER;
						goto ZL3;
					}
				}
				break;
			default:
				{
					/* BEGINNING OF ACTION: add-epsilon */
					{
#line 194 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIy))) {
			goto ZL3;
		}
	
#line 327 "src/libre/form/glob/parser.c"
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
#line 350 "src/libre/parser.act"

		act_state->err = RE_EXITEMS;
	
#line 342 "src/libre/form/glob/parser.c"
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
				case 14:
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
#line 358 "src/libre/parser.act"

		act_state->err = RE_EXEOF;
	
#line 369 "src/libre/form/glob/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL4:;
		}
		/* END OF INLINE: 58 */
	}
	return;
ZL1:;
	SAVE_LEXER (16);
	return;
}

static void
p_list_Hof_Hitems_C_Cliteral(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == 16) {
		return;
	}
	{
		t_char ZIc;

		switch (CURRENT_TERMINAL) {
		case 13:
			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 70 "src/libre/parser.act"

		ZIc = act_state->lex_tokval(lex_state);
	
#line 400 "src/libre/form/glob/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: add-literal */
		{
#line 200 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		/* TODO: check c is legal? */

		if (!fsm_addedge_literal(fsm, (ZIx), (ZIy), (ZIc))) {
			goto ZL1;
		}
	
#line 421 "src/libre/form/glob/parser.c"
		}
		/* END OF ACTION: add-literal */
	}
	return;
ZL1:;
	SAVE_LEXER (16);
	return;
}

/* BEGINNING OF TRAILER */

#line 362 "src/libre/parser.act"


#line 436 "src/libre/form/glob/parser.c"

/* END OF FILE */
