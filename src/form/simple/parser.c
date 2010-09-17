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
static void p_list_Hof_Halts_C_Citem(re, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_80(re, lex_state, act_state, t_grp, t_grp *);
static void p_86(re, lex_state, act_state, t_fsm__state, t_fsm__state, t_fsm__state *, t_fsm__state *);
static void p_89(re, lex_state, act_state, t_grp *, t_char *);
static void p_list_Hof_Halts_C_Calt(re, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_literal(re, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_group_C_Clist_Hof_Hterms_C_Cterm(re, lex_state, act_state, t_grp);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Halts(re re, lex_state lex_state, act_state act_state, t_fsm__state ZI82, t_fsm__state ZI83)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_fsm__state ZI84;
		t_fsm__state ZI85;

		p_list_Hof_Halts_C_Calt (re, lex_state, act_state, ZI82, ZI83);
		p_86 (re, lex_state, act_state, ZI82, ZI83, &ZI84, &ZI85);
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
		t_fsm__state ZIz;

		/* BEGINNING OF ACTION: add-concat */
		{
#line 191 "libre/parser.act"

		(ZIz) = fsm_addstate(re->fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 225 "form/simple/parser.c"
		}
		/* END OF ACTION: add-concat */
		p_list_Hof_Halts_C_Citem (re, lex_state, act_state, ZIx, ZIz);
		/* BEGINNING OF INLINE: 71 */
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
#line 198 "libre/parser.act"

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
		/* END OF INLINE: 71 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_group_C_Clist_Hof_Hterms(re re, lex_state lex_state, act_state act_state, t_grp ZI78)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_grp ZI79;

		p_group_C_Clist_Hof_Hterms_C_Cterm (re, lex_state, act_state, ZI78);
		p_80 (re, lex_state, act_state, ZI78, &ZI79);
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
	
#line 323 "form/simple/parser.c"
		}
		/* END OF ACTION: make-states */
		/* BEGINNING OF INLINE: 75 */
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
#line 289 "libre/parser.act"

		act_state->err = RE_EXALTS;
	
#line 364 "form/simple/parser.c"
				}
				/* END OF ACTION: err-expected-alts */
			}
		ZL2:;
		}
		/* END OF INLINE: 75 */
		/* BEGINNING OF INLINE: 76 */
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
	
#line 391 "form/simple/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL4:;
		}
		/* END OF INLINE: 76 */
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
	
#line 536 "form/simple/parser.c"
					}
					/* END OF EXTRACT: COUNT */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-n */
					{
#line 245 "libre/parser.act"

		/* TODO: check n > 0 */
		/* TODO: do this by duplicating the transitions? */
	
#line 547 "form/simple/parser.c"
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
	
#line 563 "form/simple/parser.c"
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
	
#line 579 "form/simple/parser.c"
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
	
#line 599 "form/simple/parser.c"
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
	
#line 613 "form/simple/parser.c"
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
	
#line 628 "form/simple/parser.c"
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
p_80(re re, lex_state lex_state, act_state act_state, t_grp ZI78, t_grp *ZO79)
{
	t_grp ZI79;

ZL2_80:;
	switch (CURRENT_TERMINAL) {
	case (TOK_CHAR):
		{
			p_group_C_Clist_Hof_Hterms_C_Cterm (re, lex_state, act_state, ZI78);
			/* BEGINNING OF INLINE: 80 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_80;
			}
			/* END OF INLINE: 80 */
		}
		/*UNREACHED*/
	default:
		{
			ZI79 = ZI78;
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
	*ZO79 = ZI79;
}

static void
p_86(re re, lex_state lex_state, act_state act_state, t_fsm__state ZI82, t_fsm__state ZI83, t_fsm__state *ZO84, t_fsm__state *ZO85)
{
	t_fsm__state ZI84;
	t_fsm__state ZI85;

ZL2_86:;
	switch (CURRENT_TERMINAL) {
	case (TOK_ALT):
		{
			ADVANCE_LEXER;
			p_list_Hof_Halts_C_Calt (re, lex_state, act_state, ZI82, ZI83);
			/* BEGINNING OF INLINE: 86 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_86;
			}
			/* END OF INLINE: 86 */
		}
		/*UNREACHED*/
	default:
		{
			ZI84 = ZI82;
			ZI85 = ZI83;
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
	*ZO84 = ZI84;
	*ZO85 = ZI85;
}

static void
p_89(re re, lex_state lex_state, act_state act_state, t_grp *ZIg, t_char *ZI88)
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
	
#line 735 "form/simple/parser.c"
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

		for (i = (unsigned char) (*ZI88); i <= (unsigned char) (ZIb); i++) {
			(*ZIg)[i] = 1;
		}
	
#line 755 "form/simple/parser.c"
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

		(*ZIg)[(unsigned char) (*ZI88)] = 1;
	
#line 770 "form/simple/parser.c"
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
		t_fsm__state ZIz;

		/* BEGINNING OF ACTION: add-concat */
		{
#line 191 "libre/parser.act"

		(ZIz) = fsm_addstate(re->fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 802 "form/simple/parser.c"
		}
		/* END OF ACTION: add-concat */
		/* BEGINNING OF ACTION: add-epsilon */
		{
#line 198 "libre/parser.act"

		if (!fsm_addedge_epsilon(re->fsm, (ZIx), (ZIz))) {
			goto ZL1;
		}
	
#line 813 "form/simple/parser.c"
		}
		/* END OF ACTION: add-epsilon */
		p_list_Hof_Halts_C_Clist_Hof_Hitems (re, lex_state, act_state, ZIz, ZIy);
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
	
#line 848 "form/simple/parser.c"
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
	
#line 862 "form/simple/parser.c"
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
	
#line 876 "form/simple/parser.c"
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
	
#line 900 "form/simple/parser.c"
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
		t_char ZI88;

		switch (CURRENT_TERMINAL) {
		case (TOK_CHAR):
			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 77 "libre/parser.act"

		ZI88 = act_state->lex_tokval(lex_state);
	
#line 927 "form/simple/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		p_89 (re, lex_state, act_state, &ZIg, &ZI88);
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
	
#line 950 "form/simple/parser.c"
		}
		/* END OF ACTION: err-expected-term */
	}
}

/* BEGINNING OF TRAILER */

#line 297 "libre/parser.act"


#line 961 "form/simple/parser.c"

/* END OF FILE */
