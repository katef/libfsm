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

	typedef char     t_char;
	typedef unsigned t_unsigned;
	typedef char *   t_grp;

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
static void p_list_Hof_Halts_C_Citem(re, lex_state, act_state, t_fsm__state, t_fsm__state *);
static void p_82(re, lex_state, act_state, t_grp, t_grp *);
static void p_88(re, lex_state, act_state, t_fsm__state, t_fsm__state, t_fsm__state *, t_fsm__state *);
static void p_91(re, lex_state, act_state, t_grp *, t_char *);
static void p_list_Hof_Halts_C_Calt(re, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_literal(re, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_group_C_Clist_Hof_Hterms_C_Cterm(re, lex_state, act_state, t_grp);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Halts(re re, lex_state lex_state, act_state act_state, t_fsm__state ZI84, t_fsm__state ZI85)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_fsm__state ZI86;
		t_fsm__state ZI87;

		p_list_Hof_Halts_C_Calt (re, lex_state, act_state, ZI84, ZI85);
		p_88 (re, lex_state, act_state, ZI84, ZI85, &ZI86, &ZI87);
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
#line 117 "libre/parser.act"

		(ZIg) = calloc(UCHAR_MAX, sizeof *(ZIg));
		if ((ZIg) == NULL) {
			goto ZL1;
		}
	
#line 112 "form/simple/parser.c"
		}
		/* END OF ACTION: make-group */
		/* BEGINNING OF INLINE: 60 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_SOL):
				{
					t_char ZI61;

					/* BEGINNING OF EXTRACT: SOL */
					{
#line 86 "libre/parser.act"

		ZI61 = 1;	/* TODO */
	
#line 128 "form/simple/parser.c"
					}
					/* END OF EXTRACT: SOL */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: invert-group */
					{
#line 132 "libre/parser.act"

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
		/* END OF INLINE: 60 */
		p_group_C_Clist_Hof_Hterms (re, lex_state, act_state, ZIg);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
		/* BEGINNING OF ACTION: group-to-states */
		{
#line 158 "libre/parser.act"

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
#line 124 "libre/parser.act"

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
		t_fsm__state ZIz;

		/* BEGINNING OF ACTION: add-concat */
		{
#line 192 "libre/parser.act"

		(ZIz) = fsm_addstate(re->fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 225 "form/simple/parser.c"
		}
		/* END OF ACTION: add-concat */
		p_list_Hof_Halts_C_Citem (re, lex_state, act_state, ZIx, &ZIz);
		/* BEGINNING OF INLINE: 73 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_SOL): case (TOK_EOL): case (TOK_DOT): case (TOK_OPEN__SUB):
			case (TOK_OPEN__GROUP): case (TOK_CHAR):
				{
					/* BEGINNING OF INLINE: list-of-alts::list-of-items */
					ZIx = ZIz;
					goto ZL2_list_Hof_Halts_C_Clist_Hof_Hitems;
					/* END OF INLINE: list-of-alts::list-of-items */
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
	
#line 251 "form/simple/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 73 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_group_C_Clist_Hof_Hterms(re re, lex_state lex_state, act_state act_state, t_grp ZI80)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_grp ZI81;

		p_group_C_Clist_Hof_Hterms_C_Cterm (re, lex_state, act_state, ZI80);
		p_82 (re, lex_state, act_state, ZI80, &ZI81);
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
	
#line 323 "form/simple/parser.c"
		}
		/* END OF ACTION: make-states */
		/* BEGINNING OF INLINE: 77 */
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
#line 199 "libre/parser.act"

		if (!fsm_addedge_epsilon(re->fsm, (ZIx), (ZIy))) {
			goto ZL3;
		}
	
#line 349 "form/simple/parser.c"
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
#line 344 "libre/parser.act"

		act_state->err = RE_EXALTS;
	
#line 364 "form/simple/parser.c"
				}
				/* END OF ACTION: err-expected-alts */
			}
		ZL2:;
		}
		/* END OF INLINE: 77 */
		/* BEGINNING OF INLINE: 78 */
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
	
#line 391 "form/simple/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL4:;
		}
		/* END OF INLINE: 78 */
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
#line 216 "libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		if (!fsm_addedge_any(re->fsm, (ZIx), (ZIy))) {
			goto ZL1;
		}
	
#line 430 "form/simple/parser.c"
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
p_list_Hof_Halts_C_Citem(re re, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state *ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 67 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_DOT):
				{
					p_any (re, lex_state, act_state, ZIx, *ZIy);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_OPEN__GROUP):
				{
					p_group (re, lex_state, act_state, ZIx, *ZIy);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_SOL): case (TOK_EOL): case (TOK_CHAR):
				{
					p_literal (re, lex_state, act_state, ZIx, *ZIy);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_OPEN__SUB):
				{
					p_sub (re, lex_state, act_state, ZIx, *ZIy);
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
		/* END OF INLINE: 67 */
		/* BEGINNING OF INLINE: 68 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_OPEN__COUNT):
				{
					t_unsigned ZIn;

					ADVANCE_LEXER;
					switch (CURRENT_TERMINAL) {
					case (TOK_COUNT):
						/* BEGINNING OF EXTRACT: COUNT */
						{
#line 90 "libre/parser.act"

		ZIn = act_state->lex_tokval_u(lex_state);
	
#line 539 "form/simple/parser.c"
						}
						/* END OF EXTRACT: COUNT */
						break;
					default:
						goto ZL4;
					}
					ADVANCE_LEXER;
					switch (CURRENT_TERMINAL) {
					case (TOK_CLOSE__COUNT):
						break;
					default:
						goto ZL4;
					}
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-n */
					{
#line 247 "libre/parser.act"

		struct fsm_state *a;
		struct fsm_state *b;

		if ((ZIn) == 0) {
			goto ZL4;
		}

		b = (*ZIy);

		while (--(ZIn) > 0) {
			a = fsm_state_duplicatesubgraphx(re->fsm, (ZIx), &b);
			if (a == NULL) {
				goto ZL4;
			}

			if (!fsm_addedge_epsilon(re->fsm, (*ZIy), a)) {
				goto ZL4;
			}

			(*ZIy) = b;
			(ZIx) = a;
		}
	
#line 581 "form/simple/parser.c"
					}
					/* END OF ACTION: count-n */
				}
				break;
			case (TOK_PLUS):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-1-or-many */
					{
#line 302 "libre/parser.act"

		if (!fsm_addedge_epsilon(re->fsm, (*ZIy), (ZIx))) {
			goto ZL4;
		}

		/* isolation guard */
		/* TODO: centralise */
		{
			struct fsm_state *z;

			z = fsm_addstate(re->fsm);
			if (z == NULL) {
				goto ZL4;
			}

			if (!fsm_addedge_epsilon(re->fsm, (*ZIy), z)) {
				goto ZL4;
			}

			(*ZIy) = z;
		}
	
#line 614 "form/simple/parser.c"
					}
					/* END OF ACTION: count-1-or-many */
				}
				break;
			case (TOK_QMARK):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-0-or-1 */
					{
#line 269 "libre/parser.act"

		if (!fsm_addedge_epsilon(re->fsm, (ZIx), (*ZIy))) {
			goto ZL4;
		}
	
#line 630 "form/simple/parser.c"
					}
					/* END OF ACTION: count-0-or-1 */
				}
				break;
			case (TOK_STAR):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-0-or-many */
					{
#line 275 "libre/parser.act"

		if (!fsm_addedge_epsilon(re->fsm, (ZIx), (*ZIy))) {
			goto ZL4;
		}

		if (!fsm_addedge_epsilon(re->fsm, (*ZIy), (ZIx))) {
			goto ZL4;
		}

		/* isolation guard */
		/* TODO: centralise */
		{
			struct fsm_state *z;

			z = fsm_addstate(re->fsm);
			if (z == NULL) {
				goto ZL4;
			}

			if (!fsm_addedge_epsilon(re->fsm, (*ZIy), z)) {
				goto ZL4;
			}

			(*ZIy) = z;
		}
	
#line 667 "form/simple/parser.c"
					}
					/* END OF ACTION: count-0-or-many */
				}
				break;
			default:
				{
					/* BEGINNING OF ACTION: count-1 */
					{
#line 325 "libre/parser.act"

		(void) (ZIx);
		(void) (*ZIy);
	
#line 681 "form/simple/parser.c"
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
#line 336 "libre/parser.act"

		act_state->err = RE_EXCOUNT;
	
#line 696 "form/simple/parser.c"
				}
				/* END OF ACTION: err-expected-count */
			}
		ZL3:;
		}
		/* END OF INLINE: 68 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_82(re re, lex_state lex_state, act_state act_state, t_grp ZI80, t_grp *ZO81)
{
	t_grp ZI81;

ZL2_82:;
	switch (CURRENT_TERMINAL) {
	case (TOK_CHAR):
		{
			p_group_C_Clist_Hof_Hterms_C_Cterm (re, lex_state, act_state, ZI80);
			/* BEGINNING OF INLINE: 82 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_82;
			}
			/* END OF INLINE: 82 */
		}
		/*UNREACHED*/
	default:
		{
			ZI81 = ZI80;
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
	*ZO81 = ZI81;
}

static void
p_88(re re, lex_state lex_state, act_state act_state, t_fsm__state ZI84, t_fsm__state ZI85, t_fsm__state *ZO86, t_fsm__state *ZO87)
{
	t_fsm__state ZI86;
	t_fsm__state ZI87;

ZL2_88:;
	switch (CURRENT_TERMINAL) {
	case (TOK_ALT):
		{
			ADVANCE_LEXER;
			p_list_Hof_Halts_C_Calt (re, lex_state, act_state, ZI84, ZI85);
			/* BEGINNING OF INLINE: 88 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_88;
			}
			/* END OF INLINE: 88 */
		}
		/*UNREACHED*/
	default:
		{
			ZI86 = ZI84;
			ZI87 = ZI85;
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
	*ZO86 = ZI86;
	*ZO87 = ZI87;
}

static void
p_91(re re, lex_state lex_state, act_state act_state, t_grp *ZIg, t_char *ZI90)
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
#line 78 "libre/parser.act"

		ZIb = act_state->lex_tokval(lex_state);
	
#line 803 "form/simple/parser.c"
				}
				/* END OF EXTRACT: CHAR */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: group-add-range */
			{
#line 148 "libre/parser.act"

		int i;

		assert((*ZIg) != NULL);

		for (i = (unsigned char) (*ZI90); i <= (unsigned char) (ZIb); i++) {
			(*ZIg)[i] = 1;
		}
	
#line 823 "form/simple/parser.c"
			}
			/* END OF ACTION: group-add-range */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: group-add-char */
			{
#line 140 "libre/parser.act"

		assert((*ZIg) != NULL);

		(*ZIg)[(unsigned char) (*ZI90)] = 1;
	
#line 838 "form/simple/parser.c"
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

		/* BEGINNING OF INLINE: 63 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 78 "libre/parser.act"

		ZIc = act_state->lex_tokval(lex_state);
	
#line 891 "form/simple/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_EOL):
				{
					/* BEGINNING OF EXTRACT: EOL */
					{
#line 82 "libre/parser.act"

		ZIc = 0;	/* TODO */
	
#line 905 "form/simple/parser.c"
					}
					/* END OF EXTRACT: EOL */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_SOL):
				{
					/* BEGINNING OF EXTRACT: SOL */
					{
#line 86 "libre/parser.act"

		ZIc = 1;	/* TODO */
	
#line 919 "form/simple/parser.c"
					}
					/* END OF EXTRACT: SOL */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 63 */
		/* BEGINNING OF ACTION: add-literal */
		{
#line 205 "libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		/* TODO: check c is legal? */

		if (!fsm_addedge_literal(re->fsm, (ZIx), (ZIy), (ZIc))) {
			goto ZL1;
		}
	
#line 943 "form/simple/parser.c"
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
		t_char ZI90;

		switch (CURRENT_TERMINAL) {
		case (TOK_CHAR):
			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 78 "libre/parser.act"

		ZI90 = act_state->lex_tokval(lex_state);
	
#line 970 "form/simple/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		p_91 (re, lex_state, act_state, &ZIg, &ZI90);
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
#line 332 "libre/parser.act"

		act_state->err = RE_EXTERM;
	
#line 993 "form/simple/parser.c"
		}
		/* END OF ACTION: err-expected-term */
	}
}

/* BEGINNING OF TRAILER */

#line 352 "libre/parser.act"


#line 1004 "form/simple/parser.c"

/* END OF FILE */
