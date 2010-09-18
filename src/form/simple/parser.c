/*
 * Automatically generated from the files:
 *	form/simple/parser.sid
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

	typedef char          t_char;
	typedef unsigned long t_unsigned;
	typedef char *        t_grp;

	typedef struct fsm_state * t_fsm__state;

#line 35 "form/simple/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Halts(re, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_group(re, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_list_Hof_Halts_C_Clist_Hof_Hitems(re, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_group_C_Clist_Hof_Hterms(re, lex_state, act_state, t_grp);
extern void p_re__simple(re, lex_state, act_state);
static void p_any(re, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_sub(re, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_79(re, lex_state, act_state, t_grp, t_grp *);
static void p_list_Hof_Halts_C_Citem(re, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_85(re, lex_state, act_state, t_fsm__state, t_fsm__state, t_fsm__state *, t_fsm__state *);
static void p_88(re, lex_state, act_state, t_grp *, t_char *);
static void p_list_Hof_Halts_C_Calt(re, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_literal(re, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_group_C_Clist_Hof_Hterms_C_Cterm(re, lex_state, act_state, t_grp);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Halts(re re, lex_state lex_state, act_state act_state, t_fsm__state ZI81, t_fsm__state ZI82)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_fsm__state ZI83;
		t_fsm__state ZI84;

		p_list_Hof_Halts_C_Calt (re, lex_state, act_state, ZI81, ZI82);
		p_85 (re, lex_state, act_state, ZI81, ZI82, &ZI83, &ZI84);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_group(re re, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_grp ZIg;

		switch (CURRENT_TERMINAL) {
		case (TOK_OPEN__GROUP):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: make-group */
		{
#line 116 "libre/parser.act"

		(ZIg) = calloc(UCHAR_MAX, sizeof *(ZIg));
		if ((ZIg) == NULL) {
			goto ZL1;
		}
	
#line 112 "form/simple/parser.c"
		}
		/* END OF ACTION: make-group */
		/* BEGINNING OF INLINE: 58 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_SOL):
				{
					t_char ZI59;

					/* BEGINNING OF EXTRACT: SOL */
					{
#line 85 "libre/parser.act"

		ZI59 = 1;	/* TODO */
	
#line 128 "form/simple/parser.c"
					}
					/* END OF EXTRACT: SOL */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: invert-group */
					{
#line 131 "libre/parser.act"

		int i;

		assert((ZIg) != NULL);

		for (i = 0; i <= UCHAR_MAX; i++) {
			(ZIg)[i] = !(ZIg)[i];
		}
	
#line 144 "form/simple/parser.c"
					}
					/* END OF ACTION: invert-group */
				}
				break;
			default:
				break;
			}
		}
		/* END OF INLINE: 58 */
		p_group_C_Clist_Hof_Hterms (re, lex_state, act_state, ZIg);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
		/* BEGINNING OF ACTION: group-to-states */
		{
#line 157 "libre/parser.act"

		int i;

		assert((ZIg) != NULL);

		/* TODO: eventually libfsm will provide a neater mechanism here */
		for (i = 0; i <= UCHAR_MAX; i++) {
			if (!(ZIg)[i]) {
				continue;
			}

			if (!fsm_addedge_literal(re->fsm, (ZIx), (ZIy), (char) i)) {
				goto ZL1;
			}
		}
	
#line 178 "form/simple/parser.c"
		}
		/* END OF ACTION: group-to-states */
		/* BEGINNING OF ACTION: free-group */
		{
#line 123 "libre/parser.act"

		assert((ZIg) != NULL);

		free((ZIg));
	
#line 189 "form/simple/parser.c"
		}
		/* END OF ACTION: free-group */
		switch (CURRENT_TERMINAL) {
		case (TOK_CLOSE__GROUP):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Halts_C_Clist_Hof_Hitems(re re, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_list_Hof_Halts_C_Clist_Hof_Hitems:;
	{
		t_fsm__state ZIa;
		t_fsm__state ZIb;

		/* BEGINNING OF ACTION: add-concat */
		{
#line 191 "libre/parser.act"

		(ZIa) = fsm_addstate(re->fsm);
		if ((ZIa) == NULL) {
			goto ZL1;
		}
	
#line 226 "form/simple/parser.c"
		}
		/* END OF ACTION: add-concat */
		/* BEGINNING OF ACTION: add-concat */
		{
#line 191 "libre/parser.act"

		(ZIb) = fsm_addstate(re->fsm);
		if ((ZIb) == NULL) {
			goto ZL1;
		}
	
#line 238 "form/simple/parser.c"
		}
		/* END OF ACTION: add-concat */
		/* BEGINNING OF ACTION: add-epsilon */
		{
#line 198 "libre/parser.act"

		if (!fsm_addedge_epsilon(re->fsm, (ZIx), (ZIa))) {
			goto ZL1;
		}
	
#line 249 "form/simple/parser.c"
		}
		/* END OF ACTION: add-epsilon */
		p_list_Hof_Halts_C_Citem (re, lex_state, act_state, ZIa, ZIb);
		/* BEGINNING OF INLINE: 70 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_SOL): case (TOK_EOL): case (TOK_DOT): case (TOK_OPEN__SUB):
			case (TOK_OPEN__GROUP): case (TOK_CHAR):
				{
					/* BEGINNING OF INLINE: list-of-alts::list-of-items */
					ZIx = ZIb;
					goto ZL2_list_Hof_Halts_C_Clist_Hof_Hitems;
					/* END OF INLINE: list-of-alts::list-of-items */
				}
				/*UNREACHED*/
			default:
				{
					/* BEGINNING OF ACTION: add-epsilon */
					{
#line 198 "libre/parser.act"

		if (!fsm_addedge_epsilon(re->fsm, (ZIb), (ZIy))) {
			goto ZL1;
		}
	
#line 275 "form/simple/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 70 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_group_C_Clist_Hof_Hterms(re re, lex_state lex_state, act_state act_state, t_grp ZI77)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_grp ZI78;

		p_group_C_Clist_Hof_Hterms_C_Cterm (re, lex_state, act_state, ZI77);
		p_79 (re, lex_state, act_state, ZI77, &ZI78);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

void
p_re__simple(re re, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_fsm__state ZIx;
		t_fsm__state ZIy;

		/* BEGINNING OF ACTION: make-states */
		{
#line 99 "libre/parser.act"

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
	
#line 347 "form/simple/parser.c"
		}
		/* END OF ACTION: make-states */
		/* BEGINNING OF INLINE: 74 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_SOL): case (TOK_EOL): case (TOK_DOT): case (TOK_OPEN__SUB):
			case (TOK_OPEN__GROUP): case (TOK_CHAR):
				{
					p_list_Hof_Halts (re, lex_state, act_state, ZIx, ZIy);
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
#line 198 "libre/parser.act"

		if (!fsm_addedge_epsilon(re->fsm, (ZIx), (ZIy))) {
			goto ZL3;
		}
	
#line 373 "form/simple/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			}
			goto ZL2;
		ZL3:;
			{
				/* BEGINNING OF ACTION: err-expected-alts */
				{
#line 289 "libre/parser.act"

		act_state->err = RE_EXALTS;
	
#line 388 "form/simple/parser.c"
				}
				/* END OF ACTION: err-expected-alts */
			}
		ZL2:;
		}
		/* END OF INLINE: 74 */
		/* BEGINNING OF INLINE: 75 */
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
#line 293 "libre/parser.act"

		act_state->err = RE_EXEOF;
	
#line 415 "form/simple/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL4:;
		}
		/* END OF INLINE: 75 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_any(re re, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case (TOK_DOT):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: add-any */
		{
#line 215 "libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		if (!fsm_addedge_any(re->fsm, (ZIx), (ZIy))) {
			goto ZL1;
		}
	
#line 454 "form/simple/parser.c"
		}
		/* END OF ACTION: add-any */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_sub(re re, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case (TOK_OPEN__SUB):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		p_list_Hof_Halts (re, lex_state, act_state, ZIx, ZIy);
		switch (CURRENT_TERMINAL) {
		case (TOK_CLOSE__SUB):
			break;
		case (ERROR_TERMINAL):
			RESTORE_LEXER;
			goto ZL1;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_79(re re, lex_state lex_state, act_state act_state, t_grp ZI77, t_grp *ZO78)
{
	t_grp ZI78;

ZL2_79:;
	switch (CURRENT_TERMINAL) {
	case (TOK_CHAR):
		{
			p_group_C_Clist_Hof_Hterms_C_Cterm (re, lex_state, act_state, ZI77);
			/* BEGINNING OF INLINE: 79 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_79;
			}
			/* END OF INLINE: 79 */
		}
		/*UNREACHED*/
	default:
		{
			ZI78 = ZI77;
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZO78 = ZI78;
}

static void
p_list_Hof_Halts_C_Citem(re re, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 65 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_DOT):
				{
					p_any (re, lex_state, act_state, ZIx, ZIy);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_OPEN__GROUP):
				{
					p_group (re, lex_state, act_state, ZIx, ZIy);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_SOL): case (TOK_EOL): case (TOK_CHAR):
				{
					p_literal (re, lex_state, act_state, ZIx, ZIy);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_OPEN__SUB):
				{
					p_sub (re, lex_state, act_state, ZIx, ZIy);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 65 */
		/* BEGINNING OF INLINE: 66 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_COUNT):
				{
					t_unsigned ZIn;

					/* BEGINNING OF EXTRACT: COUNT */
					{
#line 89 "libre/parser.act"

		ZIn = 0;	/* TODO: strtoul */
	
#line 596 "form/simple/parser.c"
					}
					/* END OF EXTRACT: COUNT */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-n */
					{
#line 245 "libre/parser.act"

		/* TODO: check n > 0 */
		/* TODO: do this by duplicating the transitions? */
	
#line 607 "form/simple/parser.c"
					}
					/* END OF ACTION: count-n */
				}
				break;
			case (TOK_PLUS):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-1-or-many */
					{
#line 264 "libre/parser.act"

		if (!fsm_addedge_epsilon(re->fsm, (ZIy), (ZIx))) {
			goto ZL4;
		}
	
#line 623 "form/simple/parser.c"
					}
					/* END OF ACTION: count-1-or-many */
				}
				break;
			case (TOK_QMARK):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-0-or-1 */
					{
#line 248 "libre/parser.act"

		if (!fsm_addedge_epsilon(re->fsm, (ZIx), (ZIy))) {
			goto ZL4;
		}
	
#line 639 "form/simple/parser.c"
					}
					/* END OF ACTION: count-0-or-1 */
				}
				break;
			case (TOK_STAR):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-0-or-many */
					{
#line 254 "libre/parser.act"

		if (!fsm_addedge_epsilon(re->fsm, (ZIx), (ZIy))) {
			goto ZL4;
		}

		if (!fsm_addedge_epsilon(re->fsm, (ZIy), (ZIx))) {
			goto ZL4;
		}
	
#line 659 "form/simple/parser.c"
					}
					/* END OF ACTION: count-0-or-many */
				}
				break;
			default:
				{
					/* BEGINNING OF ACTION: count-1 */
					{
#line 270 "libre/parser.act"

		(void) (ZIx);
		(void) (ZIy);
	
#line 673 "form/simple/parser.c"
					}
					/* END OF ACTION: count-1 */
				}
				break;
			}
			goto ZL3;
		ZL4:;
			{
				/* BEGINNING OF ACTION: err-expected-count */
				{
#line 281 "libre/parser.act"

		act_state->err = RE_EXCOUNT;
	
#line 688 "form/simple/parser.c"
				}
				/* END OF ACTION: err-expected-count */
			}
		ZL3:;
		}
		/* END OF INLINE: 66 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_85(re re, lex_state lex_state, act_state act_state, t_fsm__state ZI81, t_fsm__state ZI82, t_fsm__state *ZO83, t_fsm__state *ZO84)
{
	t_fsm__state ZI83;
	t_fsm__state ZI84;

ZL2_85:;
	switch (CURRENT_TERMINAL) {
	case (TOK_ALT):
		{
			ADVANCE_LEXER;
			p_list_Hof_Halts_C_Calt (re, lex_state, act_state, ZI81, ZI82);
			/* BEGINNING OF INLINE: 85 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_85;
			}
			/* END OF INLINE: 85 */
		}
		/*UNREACHED*/
	default:
		{
			ZI83 = ZI81;
			ZI84 = ZI82;
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZO83 = ZI83;
	*ZO84 = ZI84;
}

static void
p_88(re re, lex_state lex_state, act_state act_state, t_grp *ZIg, t_char *ZI87)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_SEP):
		{
			t_char ZIb;

			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				/* BEGINNING OF EXTRACT: CHAR */
				{
#line 77 "libre/parser.act"

		ZIb = act_state->lex_tokval(lex_state);
	
#line 759 "form/simple/parser.c"
				}
				/* END OF EXTRACT: CHAR */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: group-add-range */
			{
#line 147 "libre/parser.act"

		int i;

		assert((*ZIg) != NULL);

		for (i = (unsigned char) (*ZI87); i <= (unsigned char) (ZIb); i++) {
			(*ZIg)[i] = 1;
		}
	
#line 779 "form/simple/parser.c"
			}
			/* END OF ACTION: group-add-range */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: group-add-char */
			{
#line 139 "libre/parser.act"

		assert((*ZIg) != NULL);

		(*ZIg)[(unsigned char) (*ZI87)] = 1;
	
#line 794 "form/simple/parser.c"
			}
			/* END OF ACTION: group-add-char */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Halts_C_Calt(re re, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_list_Hof_Halts_C_Clist_Hof_Hitems (re, lex_state, act_state, ZIx, ZIy);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_literal(re re, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZIc;

		/* BEGINNING OF INLINE: 61 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 77 "libre/parser.act"

		ZIc = act_state->lex_tokval(lex_state);
	
#line 847 "form/simple/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_EOL):
				{
					/* BEGINNING OF EXTRACT: EOL */
					{
#line 81 "libre/parser.act"

		ZIc = 0;	/* TODO */
	
#line 861 "form/simple/parser.c"
					}
					/* END OF EXTRACT: EOL */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_SOL):
				{
					/* BEGINNING OF EXTRACT: SOL */
					{
#line 85 "libre/parser.act"

		ZIc = 1;	/* TODO */
	
#line 875 "form/simple/parser.c"
					}
					/* END OF EXTRACT: SOL */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 61 */
		/* BEGINNING OF ACTION: add-literal */
		{
#line 204 "libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		/* TODO: check c is legal? */

		if (!fsm_addedge_literal(re->fsm, (ZIx), (ZIy), (ZIc))) {
			goto ZL1;
		}
	
#line 899 "form/simple/parser.c"
		}
		/* END OF ACTION: add-literal */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_group_C_Clist_Hof_Hterms_C_Cterm(re re, lex_state lex_state, act_state act_state, t_grp ZIg)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZI87;

		switch (CURRENT_TERMINAL) {
		case (TOK_CHAR):
			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 77 "libre/parser.act"

		ZI87 = act_state->lex_tokval(lex_state);
	
#line 926 "form/simple/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		p_88 (re, lex_state, act_state, &ZIg, &ZI87);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-term */
		{
#line 277 "libre/parser.act"

		act_state->err = RE_EXTERM;
	
#line 949 "form/simple/parser.c"
		}
		/* END OF ACTION: err-expected-term */
	}
}

/* BEGINNING OF TRAILER */

#line 297 "libre/parser.act"


#line 960 "form/simple/parser.c"

/* END OF FILE */
