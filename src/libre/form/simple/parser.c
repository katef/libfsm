/*
 * Automatically generated from the files:
 *	src/libre/form/simple/parser.sid
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

#line 34 "src/libre/form/simple/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_anchor(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_expr_C_Clist_Hof_Halts_C_Calt(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_group(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_group_C_Cgroup_Hbody(fsm, cflags, lex_state, act_state, t_grp);
static void p_expr(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_group_C_Clist_Hof_Hterms(fsm, cflags, lex_state, act_state, t_grp);
extern void p_re__simple(fsm, cflags, lex_state, act_state);
static void p_expr_C_Clist_Hof_Hatoms(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_expr_C_Clist_Hof_Halts(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_97(fsm, cflags, lex_state, act_state, t_grp, t_grp *);
static void p_103(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state, t_fsm__state *, t_fsm__state *);
static void p_106(fsm, cflags, lex_state, act_state, t_grp *);
static void p_expr_C_Catom(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state *);
static void p_108(fsm, cflags, lex_state, act_state, t_grp *, t_char *);
static void p_110(fsm, cflags, lex_state, act_state, t_fsm__state *, t_fsm__state *, t_unsigned *);
static void p_literal(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_group_C_Clist_Hof_Hterms_C_Cterm(fsm, cflags, lex_state, act_state, t_grp);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_anchor(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_END):
		{
			t_pred ZIp;

			/* BEGINNING OF EXTRACT: END */
			{
#line 128 "src/libre/parser.act"

		switch (cflags & RE_ANCHOR) {
		case RE_TEXT:  ZIp = RE_EOT; break;
		case RE_MULTI: ZIp = RE_EOL; break;
		case RE_ZONE:  ZIp = RE_EOZ; break;

		default:
			/* TODO: raise error */
			ZIp = 0U;
		}
	
#line 88 "src/libre/form/simple/parser.c"
			}
			/* END OF EXTRACT: END */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: add-pred */
			{
#line 258 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

/* TODO:
		if (!fsm_addedge_predicate(fsm, (ZIx), (ZIy), (ZIp))) {
			goto ZL1;
		}
*/
	
#line 105 "src/libre/form/simple/parser.c"
			}
			/* END OF ACTION: add-pred */
		}
		break;
	case (TOK_START):
		{
			t_pred ZIp;

			/* BEGINNING OF EXTRACT: START */
			{
#line 116 "src/libre/parser.act"

		switch (cflags & RE_ANCHOR) {
		case RE_TEXT:  ZIp = RE_SOT; break;
		case RE_MULTI: ZIp = RE_SOL; break;
		case RE_ZONE:  ZIp = RE_SOZ; break;

		default:
			/* TODO: raise error */
			ZIp = 0U;
		}
	
#line 128 "src/libre/form/simple/parser.c"
			}
			/* END OF EXTRACT: START */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: add-pred */
			{
#line 258 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

/* TODO:
		if (!fsm_addedge_predicate(fsm, (ZIx), (ZIy), (ZIp))) {
			goto ZL1;
		}
*/
	
#line 145 "src/libre/form/simple/parser.c"
			}
			/* END OF ACTION: add-pred */
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
p_expr_C_Clist_Hof_Halts_C_Calt(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_fsm__state ZIz;

		/* BEGINNING OF ACTION: add-concat */
		{
#line 245 "src/libre/parser.act"

		(ZIz) = fsm_addstate(fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 179 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: add-concat */
		/* BEGINNING OF ACTION: add-epsilon */
		{
#line 252 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIz))) {
			goto ZL1;
		}
	
#line 190 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: add-epsilon */
		p_expr_C_Clist_Hof_Hatoms (fsm, cflags, lex_state, act_state, ZIz, ZIy);
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
p_group(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_grp ZIg;
		t_char ZI71;

		switch (CURRENT_TERMINAL) {
		case (TOK_OPENGROUP):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: make-group */
		{
#line 170 "src/libre/parser.act"

		(ZIg) = calloc(UCHAR_MAX + 1, sizeof *(ZIg));
		if ((ZIg) == NULL) {
			goto ZL1;
		}
	
#line 231 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: make-group */
		/* BEGINNING OF INLINE: 69 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_INVERT):
				{
					t_char ZI70;

					/* BEGINNING OF EXTRACT: INVERT */
					{
#line 79 "src/libre/parser.act"

		ZI70 = '^';
	
#line 247 "src/libre/form/simple/parser.c"
					}
					/* END OF EXTRACT: INVERT */
					ADVANCE_LEXER;
					p_group_C_Cgroup_Hbody (fsm, cflags, lex_state, act_state, ZIg);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
					/* BEGINNING OF ACTION: invert-group */
					{
#line 185 "src/libre/parser.act"

		int i;

		assert((ZIg) != NULL);

		for (i = 0; i <= UCHAR_MAX; i++) {
			(ZIg)[i] = !(ZIg)[i];
		}
	
#line 268 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: invert-group */
				}
				break;
			case (TOK_CLOSEGROUP): case (TOK_RANGE): case (TOK_ESC): case (TOK_CHAR):
				{
					p_group_C_Cgroup_Hbody (fsm, cflags, lex_state, act_state, ZIg);
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
		/* END OF INLINE: 69 */
		/* BEGINNING OF ACTION: group-to-states */
		{
#line 211 "src/libre/parser.act"

		int i;

		assert((ZIg) != NULL);

		/* TODO: eventually libfsm will provide a neater mechanism here */
		for (i = 0; i <= UCHAR_MAX; i++) {
			if (!(ZIg)[i]) {
				continue;
			}

			if (!fsm_addedge_literal(fsm, (ZIx), (ZIy), (char) i)) {
				goto ZL1;
			}
		}
	
#line 306 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: group-to-states */
		/* BEGINNING OF ACTION: free-group */
		{
#line 177 "src/libre/parser.act"

		assert((ZIg) != NULL);

		free((ZIg));
	
#line 317 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: free-group */
		switch (CURRENT_TERMINAL) {
		case (TOK_CLOSEGROUP):
			/* BEGINNING OF EXTRACT: CLOSEGROUP */
			{
#line 87 "src/libre/parser.act"

		ZI71 = ']';
	
#line 328 "src/libre/form/simple/parser.c"
			}
			/* END OF EXTRACT: CLOSEGROUP */
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
p_group_C_Cgroup_Hbody(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_grp ZIg)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 67 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSEGROUP):
				{
					t_char ZI105;

					/* BEGINNING OF EXTRACT: CLOSEGROUP */
					{
#line 87 "src/libre/parser.act"

		ZI105 = ']';
	
#line 363 "src/libre/form/simple/parser.c"
					}
					/* END OF EXTRACT: CLOSEGROUP */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: group-add-char */
					{
#line 193 "src/libre/parser.act"

		assert((ZIg) != NULL);

		(ZIg)[(unsigned char) (ZI105)] = 1;
	
#line 375 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: group-add-char */
					p_106 (fsm, cflags, lex_state, act_state, &ZIg);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_RANGE):
				{
					t_char ZIc;

					/* BEGINNING OF EXTRACT: RANGE */
					{
#line 83 "src/libre/parser.act"

		ZIc = '-';
	
#line 395 "src/libre/form/simple/parser.c"
					}
					/* END OF EXTRACT: RANGE */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: group-add-char */
					{
#line 193 "src/libre/parser.act"

		assert((ZIg) != NULL);

		(ZIg)[(unsigned char) (ZIc)] = 1;
	
#line 407 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: group-add-char */
				}
				break;
			default:
				break;
			}
		}
		/* END OF INLINE: 67 */
		p_group_C_Clist_Hof_Hterms (fsm, cflags, lex_state, act_state, ZIg);
		/* BEGINNING OF INLINE: 68 */
		{
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 68 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_expr(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_expr_C_Clist_Hof_Halts (fsm, cflags, lex_state, act_state, ZIx, ZIy);
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
p_group_C_Clist_Hof_Hterms(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_grp ZI95)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_grp ZI96;

		p_group_C_Clist_Hof_Hterms_C_Cterm (fsm, cflags, lex_state, act_state, ZI95);
		p_97 (fsm, cflags, lex_state, act_state, ZI95, &ZI96);
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
p_re__simple(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state)
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
	
#line 501 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: make-states */
		/* BEGINNING OF INLINE: 92 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_DOT): case (TOK_OPENSUB): case (TOK_OPENGROUP): case (TOK_ESC):
			case (TOK_CHAR): case (TOK_START): case (TOK_END):
				{
					p_expr (fsm, cflags, lex_state, act_state, ZIx, ZIy);
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
	
#line 527 "src/libre/form/simple/parser.c"
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
#line 423 "src/libre/parser.act"

		act_state->e = RE_EXALTS;
	
#line 542 "src/libre/form/simple/parser.c"
				}
				/* END OF ACTION: err-expected-alts */
			}
		ZL2:;
		}
		/* END OF INLINE: 92 */
		/* BEGINNING OF INLINE: 93 */
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
	
#line 569 "src/libre/form/simple/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL4:;
		}
		/* END OF INLINE: 93 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_expr_C_Clist_Hof_Hatoms(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_expr_C_Clist_Hof_Hatoms:;
	{
		t_fsm__state ZIz;

		/* BEGINNING OF ACTION: add-concat */
		{
#line 245 "src/libre/parser.act"

		(ZIz) = fsm_addstate(fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 602 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: add-concat */
		/* BEGINNING OF INLINE: 85 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_START): case (TOK_END):
				{
					p_anchor (fsm, cflags, lex_state, act_state, ZIx, ZIz);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_DOT): case (TOK_OPENSUB): case (TOK_OPENGROUP): case (TOK_ESC):
			case (TOK_CHAR):
				{
					p_expr_C_Catom (fsm, cflags, lex_state, act_state, ZIx, &ZIz);
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
		/* END OF INLINE: 85 */
		/* BEGINNING OF INLINE: 86 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_DOT): case (TOK_OPENSUB): case (TOK_OPENGROUP): case (TOK_ESC):
			case (TOK_CHAR): case (TOK_START): case (TOK_END):
				{
					/* BEGINNING OF INLINE: expr::list-of-atoms */
					ZIx = ZIz;
					goto ZL2_expr_C_Clist_Hof_Hatoms;
					/* END OF INLINE: expr::list-of-atoms */
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
	
#line 654 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			}
		}
		/* END OF INLINE: 86 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_expr_C_Clist_Hof_Halts(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZI99, t_fsm__state ZI100)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_fsm__state ZI101;
		t_fsm__state ZI102;

		p_expr_C_Clist_Hof_Halts_C_Calt (fsm, cflags, lex_state, act_state, ZI99, ZI100);
		p_103 (fsm, cflags, lex_state, act_state, ZI99, ZI100, &ZI101, &ZI102);
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
p_97(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_grp ZI95, t_grp *ZO96)
{
	t_grp ZI96;

ZL2_97:;
	switch (CURRENT_TERMINAL) {
	case (TOK_ESC): case (TOK_CHAR):
		{
			p_group_C_Clist_Hof_Hterms_C_Cterm (fsm, cflags, lex_state, act_state, ZI95);
			/* BEGINNING OF INLINE: 97 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_97;
			}
			/* END OF INLINE: 97 */
		}
		/*UNREACHED*/
	default:
		{
			ZI96 = ZI95;
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
	*ZO96 = ZI96;
}

static void
p_103(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZI99, t_fsm__state ZI100, t_fsm__state *ZO101, t_fsm__state *ZO102)
{
	t_fsm__state ZI101;
	t_fsm__state ZI102;

ZL2_103:;
	switch (CURRENT_TERMINAL) {
	case (TOK_ALT):
		{
			ADVANCE_LEXER;
			p_expr_C_Clist_Hof_Halts_C_Calt (fsm, cflags, lex_state, act_state, ZI99, ZI100);
			/* BEGINNING OF INLINE: 103 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_103;
			}
			/* END OF INLINE: 103 */
		}
		/*UNREACHED*/
	default:
		{
			ZI101 = ZI99;
			ZI102 = ZI100;
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
	*ZO101 = ZI101;
	*ZO102 = ZI102;
}

static void
p_106(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_grp *ZIg)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZIb;

			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 83 "src/libre/parser.act"

		ZIb = '-';
	
#line 782 "src/libre/form/simple/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: group-add-char */
			{
#line 193 "src/libre/parser.act"

		assert((*ZIg) != NULL);

		(*ZIg)[(unsigned char) (ZIb)] = 1;
	
#line 794 "src/libre/form/simple/parser.c"
			}
			/* END OF ACTION: group-add-char */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	default:
		break;
	}
}

static void
p_expr_C_Catom(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state *ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 78 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_DOT):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: add-any */
					{
#line 280 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((*ZIy) != NULL);

		if (!fsm_addedge_any(fsm, (ZIx), (*ZIy))) {
			goto ZL1;
		}
	
#line 830 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: add-any */
				}
				break;
			case (TOK_OPENSUB):
				{
					ADVANCE_LEXER;
					p_expr (fsm, cflags, lex_state, act_state, ZIx, *ZIy);
					switch (CURRENT_TERMINAL) {
					case (TOK_CLOSESUB):
						break;
					case (ERROR_TERMINAL):
						RESTORE_LEXER;
						goto ZL1;
					default:
						goto ZL1;
					}
					ADVANCE_LEXER;
				}
				break;
			case (TOK_OPENGROUP):
				{
					p_group (fsm, cflags, lex_state, act_state, ZIx, *ZIy);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_ESC): case (TOK_CHAR):
				{
					p_literal (fsm, cflags, lex_state, act_state, ZIx, *ZIy);
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
		/* END OF INLINE: 78 */
		/* BEGINNING OF INLINE: 79 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_OPENCOUNT):
				{
					t_unsigned ZI109;

					ADVANCE_LEXER;
					switch (CURRENT_TERMINAL) {
					case (TOK_COUNT):
						/* BEGINNING OF EXTRACT: COUNT */
						{
#line 139 "src/libre/parser.act"

		ZI109 = strtoul(lex_state->buf.a, NULL, 10);
		/* TODO: range check */
	
#line 891 "src/libre/form/simple/parser.c"
						}
						/* END OF EXTRACT: COUNT */
						break;
					default:
						goto ZL4;
					}
					ADVANCE_LEXER;
					p_110 (fsm, cflags, lex_state, act_state, &ZIx, ZIy, &ZI109);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL4;
					}
				}
				break;
			case (TOK_PLUS):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-1-or-many */
					{
#line 381 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (*ZIy), (ZIx))) {
			goto ZL4;
		}

		/* isolation guard */
		/* TODO: centralise */
		{
			struct fsm_state *z;

			z = fsm_addstate(fsm);
			if (z == NULL) {
				goto ZL4;
			}

			if (!fsm_addedge_epsilon(fsm, (*ZIy), z)) {
				goto ZL4;
			}

			(*ZIy) = z;
		}
	
#line 934 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: count-1-or-many */
				}
				break;
			case (TOK_QMARK):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-0-or-1 */
					{
#line 348 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (*ZIy))) {
			goto ZL4;
		}
	
#line 950 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: count-0-or-1 */
				}
				break;
			case (TOK_STAR):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-0-or-many */
					{
#line 354 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (*ZIy))) {
			goto ZL4;
		}

		if (!fsm_addedge_epsilon(fsm, (*ZIy), (ZIx))) {
			goto ZL4;
		}

		/* isolation guard */
		/* TODO: centralise */
		{
			struct fsm_state *z;

			z = fsm_addstate(fsm);
			if (z == NULL) {
				goto ZL4;
			}

			if (!fsm_addedge_epsilon(fsm, (*ZIy), z)) {
				goto ZL4;
			}

			(*ZIy) = z;
		}
	
#line 987 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: count-0-or-many */
				}
				break;
			default:
				{
					/* BEGINNING OF ACTION: count-1 */
					{
#line 404 "src/libre/parser.act"

		(void) (ZIx);
		(void) (*ZIy);
	
#line 1001 "src/libre/form/simple/parser.c"
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
#line 415 "src/libre/parser.act"

		act_state->e = RE_EXCOUNT;
	
#line 1016 "src/libre/form/simple/parser.c"
				}
				/* END OF ACTION: err-expected-count */
			}
		ZL3:;
		}
		/* END OF INLINE: 79 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_108(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_grp *ZIg, t_char *ZI107)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZI61;
			t_char ZIb;

			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 83 "src/libre/parser.act"

		ZI61 = '-';
	
#line 1045 "src/libre/form/simple/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			/* BEGINNING OF INLINE: 62 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_CHAR):
					{
						/* BEGINNING OF EXTRACT: CHAR */
						{
#line 111 "src/libre/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIb = lex_state->buf.a[0];
	
#line 1063 "src/libre/form/simple/parser.c"
						}
						/* END OF EXTRACT: CHAR */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_RANGE):
					{
						/* BEGINNING OF EXTRACT: RANGE */
						{
#line 83 "src/libre/parser.act"

		ZIb = '-';
	
#line 1077 "src/libre/form/simple/parser.c"
						}
						/* END OF EXTRACT: RANGE */
						ADVANCE_LEXER;
					}
					break;
				default:
					goto ZL1;
				}
			}
			/* END OF INLINE: 62 */
			/* BEGINNING OF ACTION: group-add-range */
			{
#line 201 "src/libre/parser.act"

		int i;

		assert((*ZIg) != NULL);

		for (i = (unsigned char) (*ZI107); i <= (unsigned char) (ZIb); i++) {
			(*ZIg)[i] = 1;
		}
	
#line 1100 "src/libre/form/simple/parser.c"
			}
			/* END OF ACTION: group-add-range */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: group-add-char */
			{
#line 193 "src/libre/parser.act"

		assert((*ZIg) != NULL);

		(*ZIg)[(unsigned char) (*ZI107)] = 1;
	
#line 1115 "src/libre/form/simple/parser.c"
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
p_110(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state *ZIx, t_fsm__state *ZIy, t_unsigned *ZI109)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_CLOSECOUNT):
		{
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: count-m-to-n */
			{
#line 314 "src/libre/parser.act"

		unsigned i;
		struct fsm_state *a;
		struct fsm_state *b;

		if ((*ZI109) == 0) {
			goto ZL1;
		}

		if ((*ZI109) < (*ZI109)) {
			goto ZL1;
		}

		b = (*ZIy);

		for (i = 1; i < (*ZI109); i++) {
			a = fsm_state_duplicatesubgraphx(fsm, (*ZIx), &b);
			if (a == NULL) {
				goto ZL1;
			}

			/* TODO: could elide this epsilon if fsm_state_duplicatesubgraphx()
			 * took an extra parameter giving it a m->new for the start state */
			if (!fsm_addedge_epsilon(fsm, (*ZIy), a)) {
				goto ZL1;
			}

			if (i >= (*ZI109)) {
				if (!fsm_addedge_epsilon(fsm, (*ZIy), b)) {
					goto ZL1;
				}
			}

			(*ZIy) = b;
			(*ZIx) = a;
		}
	
#line 1176 "src/libre/form/simple/parser.c"
			}
			/* END OF ACTION: count-m-to-n */
		}
		break;
	case (TOK_SEP):
		{
			t_unsigned ZIn;

			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (TOK_COUNT):
				/* BEGINNING OF EXTRACT: COUNT */
				{
#line 139 "src/libre/parser.act"

		ZIn = strtoul(lex_state->buf.a, NULL, 10);
		/* TODO: range check */
	
#line 1195 "src/libre/form/simple/parser.c"
				}
				/* END OF EXTRACT: COUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSECOUNT):
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: count-m-to-n */
			{
#line 314 "src/libre/parser.act"

		unsigned i;
		struct fsm_state *a;
		struct fsm_state *b;

		if ((*ZI109) == 0) {
			goto ZL1;
		}

		if ((ZIn) < (*ZI109)) {
			goto ZL1;
		}

		b = (*ZIy);

		for (i = 1; i < (ZIn); i++) {
			a = fsm_state_duplicatesubgraphx(fsm, (*ZIx), &b);
			if (a == NULL) {
				goto ZL1;
			}

			/* TODO: could elide this epsilon if fsm_state_duplicatesubgraphx()
			 * took an extra parameter giving it a m->new for the start state */
			if (!fsm_addedge_epsilon(fsm, (*ZIy), a)) {
				goto ZL1;
			}

			if (i >= (*ZI109)) {
				if (!fsm_addedge_epsilon(fsm, (*ZIy), b)) {
					goto ZL1;
				}
			}

			(*ZIy) = b;
			(*ZIx) = a;
		}
	
#line 1250 "src/libre/form/simple/parser.c"
			}
			/* END OF ACTION: count-m-to-n */
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
p_literal(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_CHAR):
		{
			t_char ZIc;

			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 111 "src/libre/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIc = lex_state->buf.a[0];
	
#line 1283 "src/libre/form/simple/parser.c"
			}
			/* END OF EXTRACT: CHAR */
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
	
#line 1300 "src/libre/form/simple/parser.c"
			}
			/* END OF ACTION: add-literal */
		}
		break;
	case (TOK_ESC):
		{
			t_char ZIc;

			/* BEGINNING OF EXTRACT: ESC */
			{
#line 95 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZIc = lex_state->buf.a[1];

		switch (ZIc) {
		case 'f': ZIc = '\f'; break;
		case 'n': ZIc = '\n'; break;
		case 'r': ZIc = '\r'; break;
		case 't': ZIc = '\t'; break;
		case 'v': ZIc = '\v'; break;
		default:             break;
		}
	
#line 1328 "src/libre/form/simple/parser.c"
			}
			/* END OF EXTRACT: ESC */
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
	
#line 1345 "src/libre/form/simple/parser.c"
			}
			/* END OF ACTION: add-literal */
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
p_group_C_Clist_Hof_Hterms_C_Cterm(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_grp ZIg)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_CHAR):
		{
			t_char ZI107;

			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 111 "src/libre/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZI107 = lex_state->buf.a[0];
	
#line 1378 "src/libre/form/simple/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			ADVANCE_LEXER;
			p_108 (fsm, cflags, lex_state, act_state, &ZIg, &ZI107);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_ESC):
		{
			t_char ZIc;

			/* BEGINNING OF EXTRACT: ESC */
			{
#line 95 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZIc = lex_state->buf.a[1];

		switch (ZIc) {
		case 'f': ZIc = '\f'; break;
		case 'n': ZIc = '\n'; break;
		case 'r': ZIc = '\r'; break;
		case 't': ZIc = '\t'; break;
		case 'v': ZIc = '\v'; break;
		default:             break;
		}
	
#line 1412 "src/libre/form/simple/parser.c"
			}
			/* END OF EXTRACT: ESC */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: group-add-char */
			{
#line 193 "src/libre/parser.act"

		assert((ZIg) != NULL);

		(ZIg)[(unsigned char) (ZIc)] = 1;
	
#line 1424 "src/libre/form/simple/parser.c"
			}
			/* END OF ACTION: group-add-char */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	default:
		goto ZL1;
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-term */
		{
#line 411 "src/libre/parser.act"

		act_state->e = RE_EXTERM;
	
#line 1443 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: err-expected-term */
	}
}

/* BEGINNING OF TRAILER */

#line 431 "src/libre/parser.act"


#line 1454 "src/libre/form/simple/parser.c"

/* END OF FILE */
