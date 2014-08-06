/*
 * Automatically generated from the files:
 *	src/libre/form/simple/parser.sid
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

#line 33 "src/libre/form/simple/parser.c"

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Halts(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_group(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_list_Hof_Halts_C_Clist_Hof_Hitems(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_group_C_Clist_Hof_Hterms(fsm, cflags, lex_state, act_state, t_grp);
extern void p_re__simple(fsm, cflags, lex_state, act_state);
static void p_any(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_sub(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_list_Hof_Halts_C_Citem(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state *);
static void p_84(fsm, cflags, lex_state, act_state, t_grp, t_grp *);
static void p_90(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state, t_fsm__state *, t_fsm__state *);
static void p_93(fsm, cflags, lex_state, act_state, t_grp *, t_char *);
static void p_95(fsm, cflags, lex_state, act_state, t_fsm__state *, t_fsm__state *, t_unsigned *);
static void p_list_Hof_Halts_C_Calt(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_literal(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_group_C_Clist_Hof_Hterms_C_Cterm(fsm, cflags, lex_state, act_state, t_grp);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Halts(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZI86, t_fsm__state ZI87)
{
	if ((CURRENT_TERMINAL) == 19) {
		return;
	}
	{
		t_fsm__state ZI88;
		t_fsm__state ZI89;

		p_list_Hof_Halts_C_Calt (fsm, cflags, lex_state, act_state, ZI86, ZI87);
		p_90 (fsm, cflags, lex_state, act_state, ZI86, ZI87, &ZI88, &ZI89);
		if ((CURRENT_TERMINAL) == 19) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	return;
ZL1:;
	SAVE_LEXER (19);
	return;
}

static void
p_group(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == 19) {
		return;
	}
	{
		t_grp ZIg;

		switch (CURRENT_TERMINAL) {
		case 11:
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: make-group */
		{
#line 112 "src/libre/parser.act"

		(ZIg) = calloc(UCHAR_MAX, sizeof *(ZIg));
		if ((ZIg) == NULL) {
			goto ZL1;
		}
	
#line 106 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: make-group */
		/* BEGINNING OF INLINE: 61 */
		{
			switch (CURRENT_TERMINAL) {
			case 0:
				{
					t_char ZI62;

					/* BEGINNING OF EXTRACT: SOL */
					{
#line 78 "src/libre/parser.act"

		ZI62 = 1;	/* TODO */
	
#line 122 "src/libre/form/simple/parser.c"
					}
					/* END OF EXTRACT: SOL */
					ADVANCE_LEXER;
					p_group_C_Clist_Hof_Hterms (fsm, cflags, lex_state, act_state, ZIg);
					if ((CURRENT_TERMINAL) == 19) {
						RESTORE_LEXER;
						goto ZL1;
					}
					/* BEGINNING OF ACTION: invert-group */
					{
#line 127 "src/libre/parser.act"

		int i;

		assert((ZIg) != NULL);

		for (i = 0; i <= UCHAR_MAX; i++) {
			(ZIg)[i] = !(ZIg)[i];
		}
	
#line 143 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: invert-group */
				}
				break;
			case 16:
				{
					p_group_C_Clist_Hof_Hterms (fsm, cflags, lex_state, act_state, ZIg);
					if ((CURRENT_TERMINAL) == 19) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 61 */
		/* BEGINNING OF ACTION: group-to-states */
		{
#line 153 "src/libre/parser.act"

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
	
#line 181 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: group-to-states */
		/* BEGINNING OF ACTION: free-group */
		{
#line 119 "src/libre/parser.act"

		assert((ZIg) != NULL);

		free((ZIg));
	
#line 192 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: free-group */
		switch (CURRENT_TERMINAL) {
		case 12:
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
	}
	return;
ZL1:;
	SAVE_LEXER (19);
	return;
}

static void
p_list_Hof_Halts_C_Clist_Hof_Hitems(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == 19) {
		return;
	}
ZL2_list_Hof_Halts_C_Clist_Hof_Hitems:;
	{
		t_fsm__state ZIz;

		/* BEGINNING OF ACTION: add-concat */
		{
#line 187 "src/libre/parser.act"

		(ZIz) = fsm_addstate(fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 228 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: add-concat */
		p_list_Hof_Halts_C_Citem (fsm, cflags, lex_state, act_state, ZIx, &ZIz);
		/* BEGINNING OF INLINE: 75 */
		{
			switch (CURRENT_TERMINAL) {
			case 0: case 1: case 5: case 9:
			case 11: case 16:
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
#line 194 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIz), (ZIy))) {
			goto ZL1;
		}
	
#line 254 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			case 19:
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 75 */
	}
	return;
ZL1:;
	SAVE_LEXER (19);
	return;
}

static void
p_group_C_Clist_Hof_Hterms(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_grp ZI82)
{
	if ((CURRENT_TERMINAL) == 19) {
		return;
	}
	{
		t_grp ZI83;

		p_group_C_Clist_Hof_Hterms_C_Cterm (fsm, cflags, lex_state, act_state, ZI82);
		p_84 (fsm, cflags, lex_state, act_state, ZI82, &ZI83);
		if ((CURRENT_TERMINAL) == 19) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	return;
ZL1:;
	SAVE_LEXER (19);
	return;
}

void
p_re__simple(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == 19) {
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
	
#line 321 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: make-states */
		/* BEGINNING OF INLINE: 79 */
		{
			switch (CURRENT_TERMINAL) {
			case 0: case 1: case 5: case 9:
			case 11: case 16:
				{
					p_list_Hof_Halts (fsm, cflags, lex_state, act_state, ZIx, ZIy);
					if ((CURRENT_TERMINAL) == 19) {
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
	
#line 347 "src/libre/form/simple/parser.c"
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
#line 354 "src/libre/parser.act"

		act_state->err = RE_EXALTS;
	
#line 362 "src/libre/form/simple/parser.c"
				}
				/* END OF ACTION: err-expected-alts */
			}
		ZL2:;
		}
		/* END OF INLINE: 79 */
		/* BEGINNING OF INLINE: 80 */
		{
			{
				switch (CURRENT_TERMINAL) {
				case 17:
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
	
#line 389 "src/libre/form/simple/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL4:;
		}
		/* END OF INLINE: 80 */
	}
	return;
ZL1:;
	SAVE_LEXER (19);
	return;
}

static void
p_any(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == 19) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case 5:
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
	
#line 428 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: add-any */
	}
	return;
ZL1:;
	SAVE_LEXER (19);
	return;
}

static void
p_sub(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == 19) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case 9:
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		p_list_Hof_Halts (fsm, cflags, lex_state, act_state, ZIx, ZIy);
		switch (CURRENT_TERMINAL) {
		case 10:
			break;
		case 19:
			RESTORE_LEXER;
			goto ZL1;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
	}
	return;
ZL1:;
	SAVE_LEXER (19);
	return;
}

static void
p_list_Hof_Halts_C_Citem(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state *ZIy)
{
	if ((CURRENT_TERMINAL) == 19) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 68 */
		{
			switch (CURRENT_TERMINAL) {
			case 5:
				{
					p_any (fsm, cflags, lex_state, act_state, ZIx, *ZIy);
					if ((CURRENT_TERMINAL) == 19) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case 11:
				{
					p_group (fsm, cflags, lex_state, act_state, ZIx, *ZIy);
					if ((CURRENT_TERMINAL) == 19) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case 0: case 1: case 16:
				{
					p_literal (fsm, cflags, lex_state, act_state, ZIx, *ZIy);
					if ((CURRENT_TERMINAL) == 19) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case 9:
				{
					p_sub (fsm, cflags, lex_state, act_state, ZIx, *ZIy);
					if ((CURRENT_TERMINAL) == 19) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 68 */
		/* BEGINNING OF INLINE: 69 */
		{
			switch (CURRENT_TERMINAL) {
			case 13:
				{
					t_unsigned ZI94;

					ADVANCE_LEXER;
					switch (CURRENT_TERMINAL) {
					case 15:
						/* BEGINNING OF EXTRACT: COUNT */
						{
#line 82 "src/libre/parser.act"

		ZI94 = act_state->lex_tokval_u(lex_state);
	
#line 537 "src/libre/form/simple/parser.c"
						}
						/* END OF EXTRACT: COUNT */
						break;
					default:
						goto ZL4;
					}
					ADVANCE_LEXER;
					p_95 (fsm, cflags, lex_state, act_state, &ZIx, ZIy, &ZI94);
					if ((CURRENT_TERMINAL) == 19) {
						RESTORE_LEXER;
						goto ZL4;
					}
				}
				break;
			case 4:
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-1-or-many */
					{
#line 312 "src/libre/parser.act"

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
	
#line 580 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: count-1-or-many */
				}
				break;
			case 2:
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-0-or-1 */
					{
#line 279 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (*ZIy))) {
			goto ZL4;
		}
	
#line 596 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: count-0-or-1 */
				}
				break;
			case 3:
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-0-or-many */
					{
#line 285 "src/libre/parser.act"

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
	
#line 633 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: count-0-or-many */
				}
				break;
			default:
				{
					/* BEGINNING OF ACTION: count-1 */
					{
#line 335 "src/libre/parser.act"

		(void) (ZIx);
		(void) (*ZIy);
	
#line 647 "src/libre/form/simple/parser.c"
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
#line 346 "src/libre/parser.act"

		act_state->err = RE_EXCOUNT;
	
#line 662 "src/libre/form/simple/parser.c"
				}
				/* END OF ACTION: err-expected-count */
			}
		ZL3:;
		}
		/* END OF INLINE: 69 */
	}
	return;
ZL1:;
	SAVE_LEXER (19);
	return;
}

static void
p_84(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_grp ZI82, t_grp *ZO83)
{
	t_grp ZI83;

ZL2_84:;
	switch (CURRENT_TERMINAL) {
	case 16:
		{
			p_group_C_Clist_Hof_Hterms_C_Cterm (fsm, cflags, lex_state, act_state, ZI82);
			/* BEGINNING OF INLINE: 84 */
			if ((CURRENT_TERMINAL) == 19) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_84;
			}
			/* END OF INLINE: 84 */
		}
		/*UNREACHED*/
	default:
		{
			ZI83 = ZI82;
		}
		break;
	case 19:
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER (19);
	return;
ZL0:;
	*ZO83 = ZI83;
}

static void
p_90(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZI86, t_fsm__state ZI87, t_fsm__state *ZO88, t_fsm__state *ZO89)
{
	t_fsm__state ZI88;
	t_fsm__state ZI89;

ZL2_90:;
	switch (CURRENT_TERMINAL) {
	case 6:
		{
			ADVANCE_LEXER;
			p_list_Hof_Halts_C_Calt (fsm, cflags, lex_state, act_state, ZI86, ZI87);
			/* BEGINNING OF INLINE: 90 */
			if ((CURRENT_TERMINAL) == 19) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_90;
			}
			/* END OF INLINE: 90 */
		}
		/*UNREACHED*/
	default:
		{
			ZI88 = ZI86;
			ZI89 = ZI87;
		}
		break;
	case 19:
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER (19);
	return;
ZL0:;
	*ZO88 = ZI88;
	*ZO89 = ZI89;
}

static void
p_93(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_grp *ZIg, t_char *ZI92)
{
	switch (CURRENT_TERMINAL) {
	case 7:
		{
			t_char ZIb;

			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case 16:
				/* BEGINNING OF EXTRACT: CHAR */
				{
#line 70 "src/libre/parser.act"

		ZIb = act_state->lex_tokval(lex_state);
	
#line 769 "src/libre/form/simple/parser.c"
				}
				/* END OF EXTRACT: CHAR */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: group-add-range */
			{
#line 143 "src/libre/parser.act"

		int i;

		assert((*ZIg) != NULL);

		for (i = (unsigned char) (*ZI92); i <= (unsigned char) (ZIb); i++) {
			(*ZIg)[i] = 1;
		}
	
#line 789 "src/libre/form/simple/parser.c"
			}
			/* END OF ACTION: group-add-range */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: group-add-char */
			{
#line 135 "src/libre/parser.act"

		assert((*ZIg) != NULL);

		(*ZIg)[(unsigned char) (*ZI92)] = 1;
	
#line 804 "src/libre/form/simple/parser.c"
			}
			/* END OF ACTION: group-add-char */
		}
		break;
	case 19:
		return;
	}
	return;
ZL1:;
	SAVE_LEXER (19);
	return;
}

static void
p_95(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state *ZIx, t_fsm__state *ZIy, t_unsigned *ZI94)
{
	switch (CURRENT_TERMINAL) {
	case 14:
		{
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: count-m-to-n */
			{
#line 245 "src/libre/parser.act"

		unsigned i;
		struct fsm_state *a;
		struct fsm_state *b;

		if ((*ZI94) == 0) {
			goto ZL1;
		}

		if ((*ZI94) < (*ZI94)) {
			goto ZL1;
		}

		b = (*ZIy);

		for (i = 1; i < (*ZI94); i++) {
			a = fsm_state_duplicatesubgraphx(fsm, (*ZIx), &b);
			if (a == NULL) {
				goto ZL1;
			}

			/* TODO: could elide this epsilon if fsm_state_duplicatesubgraphx()
			 * took an extra parameter giving it a m->new for the start state */
			if (!fsm_addedge_epsilon(fsm, (*ZIy), a)) {
				goto ZL1;
			}

			if (i >= (*ZI94)) {
				if (!fsm_addedge_epsilon(fsm, (*ZIy), b)) {
					goto ZL1;
				}
			}

			(*ZIy) = b;
			(*ZIx) = a;
		}
	
#line 865 "src/libre/form/simple/parser.c"
			}
			/* END OF ACTION: count-m-to-n */
		}
		break;
	case 8:
		{
			t_unsigned ZIn;

			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case 15:
				/* BEGINNING OF EXTRACT: COUNT */
				{
#line 82 "src/libre/parser.act"

		ZIn = act_state->lex_tokval_u(lex_state);
	
#line 883 "src/libre/form/simple/parser.c"
				}
				/* END OF EXTRACT: COUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case 14:
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: count-m-to-n */
			{
#line 245 "src/libre/parser.act"

		unsigned i;
		struct fsm_state *a;
		struct fsm_state *b;

		if ((*ZI94) == 0) {
			goto ZL1;
		}

		if ((ZIn) < (*ZI94)) {
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

			if (i >= (*ZI94)) {
				if (!fsm_addedge_epsilon(fsm, (*ZIy), b)) {
					goto ZL1;
				}
			}

			(*ZIy) = b;
			(*ZIx) = a;
		}
	
#line 938 "src/libre/form/simple/parser.c"
			}
			/* END OF ACTION: count-m-to-n */
		}
		break;
	case 19:
		return;
	default:
		goto ZL1;
	}
	return;
ZL1:;
	SAVE_LEXER (19);
	return;
}

static void
p_list_Hof_Halts_C_Calt(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == 19) {
		return;
	}
	{
		p_list_Hof_Halts_C_Clist_Hof_Hitems (fsm, cflags, lex_state, act_state, ZIx, ZIy);
		if ((CURRENT_TERMINAL) == 19) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	return;
ZL1:;
	SAVE_LEXER (19);
	return;
}

static void
p_literal(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == 19) {
		return;
	}
	{
		t_char ZIc;

		/* BEGINNING OF INLINE: 64 */
		{
			switch (CURRENT_TERMINAL) {
			case 16:
				{
					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 70 "src/libre/parser.act"

		ZIc = act_state->lex_tokval(lex_state);
	
#line 993 "src/libre/form/simple/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
				}
				break;
			case 1:
				{
					/* BEGINNING OF EXTRACT: EOL */
					{
#line 74 "src/libre/parser.act"

		ZIc = 0;	/* TODO */
	
#line 1007 "src/libre/form/simple/parser.c"
					}
					/* END OF EXTRACT: EOL */
					ADVANCE_LEXER;
				}
				break;
			case 0:
				{
					/* BEGINNING OF EXTRACT: SOL */
					{
#line 78 "src/libre/parser.act"

		ZIc = 1;	/* TODO */
	
#line 1021 "src/libre/form/simple/parser.c"
					}
					/* END OF EXTRACT: SOL */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 64 */
		/* BEGINNING OF ACTION: add-literal */
		{
#line 200 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		/* TODO: check c is legal? */

		if (!fsm_addedge_literal(fsm, (ZIx), (ZIy), (ZIc))) {
			goto ZL1;
		}
	
#line 1045 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: add-literal */
	}
	return;
ZL1:;
	SAVE_LEXER (19);
	return;
}

static void
p_group_C_Clist_Hof_Hterms_C_Cterm(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_grp ZIg)
{
	if ((CURRENT_TERMINAL) == 19) {
		return;
	}
	{
		t_char ZI92;

		switch (CURRENT_TERMINAL) {
		case 16:
			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 70 "src/libre/parser.act"

		ZI92 = act_state->lex_tokval(lex_state);
	
#line 1072 "src/libre/form/simple/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		p_93 (fsm, cflags, lex_state, act_state, &ZIg, &ZI92);
		if ((CURRENT_TERMINAL) == 19) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-term */
		{
#line 342 "src/libre/parser.act"

		act_state->err = RE_EXTERM;
	
#line 1095 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: err-expected-term */
	}
}

/* BEGINNING OF TRAILER */

#line 362 "src/libre/parser.act"


#line 1106 "src/libre/form/simple/parser.c"

/* END OF FILE */
