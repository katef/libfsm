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


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Halts(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_group(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_group_C_Cgroup_Hbody(fsm, cflags, lex_state, act_state, t_grp);
static void p_list_Hof_Halts_C_Clist_Hof_Hatoms(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_group_C_Clist_Hof_Hterms(fsm, cflags, lex_state, act_state, t_grp);
extern void p_re__simple(fsm, cflags, lex_state, act_state);
static void p_list_Hof_Halts_C_Catom(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state *);
static void p_91(fsm, cflags, lex_state, act_state, t_grp, t_grp *);
static void p_list_Hof_Halts_C_Calt(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_97(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state, t_fsm__state *, t_fsm__state *);
static void p_100(fsm, cflags, lex_state, act_state, t_grp *);
static void p_102(fsm, cflags, lex_state, act_state, t_grp *, t_char *);
static void p_104(fsm, cflags, lex_state, act_state, t_fsm__state *, t_fsm__state *, t_unsigned *);
static void p_group_C_Clist_Hof_Hterms_C_Cterm(fsm, cflags, lex_state, act_state, t_grp);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Halts(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZI93, t_fsm__state ZI94)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_fsm__state ZI95;
		t_fsm__state ZI96;

		p_list_Hof_Halts_C_Calt (fsm, cflags, lex_state, act_state, ZI93, ZI94);
		p_97 (fsm, cflags, lex_state, act_state, ZI93, ZI94, &ZI95, &ZI96);
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
		t_char ZI69;

		switch (CURRENT_TERMINAL) {
		case (TOK_OPENGROUP):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: make-group */
		{
#line 153 "src/libre/parser.act"

		(ZIg) = calloc(UCHAR_MAX, sizeof *(ZIg));
		if ((ZIg) == NULL) {
			goto ZL1;
		}
	
#line 111 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: make-group */
		/* BEGINNING OF INLINE: 67 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_INVERT):
				{
					t_char ZI68;

					/* BEGINNING OF EXTRACT: INVERT */
					{
#line 78 "src/libre/parser.act"

		ZI68 = '^';
	
#line 127 "src/libre/form/simple/parser.c"
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
#line 168 "src/libre/parser.act"

		int i;

		assert((ZIg) != NULL);

		for (i = 0; i <= UCHAR_MAX; i++) {
			(ZIg)[i] = !(ZIg)[i];
		}
	
#line 148 "src/libre/form/simple/parser.c"
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
		/* END OF INLINE: 67 */
		/* BEGINNING OF ACTION: group-to-states */
		{
#line 194 "src/libre/parser.act"

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
	
#line 186 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: group-to-states */
		/* BEGINNING OF ACTION: free-group */
		{
#line 160 "src/libre/parser.act"

		assert((ZIg) != NULL);

		free((ZIg));
	
#line 197 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: free-group */
		switch (CURRENT_TERMINAL) {
		case (TOK_CLOSEGROUP):
			/* BEGINNING OF EXTRACT: CLOSEGROUP */
			{
#line 86 "src/libre/parser.act"

		ZI69 = ']';
	
#line 208 "src/libre/form/simple/parser.c"
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
		/* BEGINNING OF INLINE: 65 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSEGROUP):
				{
					t_char ZI99;

					/* BEGINNING OF EXTRACT: CLOSEGROUP */
					{
#line 86 "src/libre/parser.act"

		ZI99 = ']';
	
#line 243 "src/libre/form/simple/parser.c"
					}
					/* END OF EXTRACT: CLOSEGROUP */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: group-add-char */
					{
#line 176 "src/libre/parser.act"

		assert((ZIg) != NULL);

		(ZIg)[(unsigned char) (ZI99)] = 1;
	
#line 255 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: group-add-char */
					p_100 (fsm, cflags, lex_state, act_state, &ZIg);
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
#line 82 "src/libre/parser.act"

		ZIc = '-';
	
#line 275 "src/libre/form/simple/parser.c"
					}
					/* END OF EXTRACT: RANGE */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: group-add-char */
					{
#line 176 "src/libre/parser.act"

		assert((ZIg) != NULL);

		(ZIg)[(unsigned char) (ZIc)] = 1;
	
#line 287 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: group-add-char */
				}
				break;
			default:
				break;
			}
		}
		/* END OF INLINE: 65 */
		p_group_C_Clist_Hof_Hterms (fsm, cflags, lex_state, act_state, ZIg);
		/* BEGINNING OF INLINE: 66 */
		{
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 66 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Halts_C_Clist_Hof_Hatoms(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_list_Hof_Halts_C_Clist_Hof_Hatoms:;
	{
		t_fsm__state ZIz;

		/* BEGINNING OF ACTION: add-concat */
		{
#line 228 "src/libre/parser.act"

		(ZIz) = fsm_addstate(fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 332 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: add-concat */
		p_list_Hof_Halts_C_Catom (fsm, cflags, lex_state, act_state, ZIx, &ZIz);
		/* BEGINNING OF INLINE: 82 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_DOT): case (TOK_OPENSUB): case (TOK_OPENGROUP): case (TOK_ESC):
			case (TOK_CHAR): case (TOK_START): case (TOK_END):
				{
					/* BEGINNING OF INLINE: list-of-alts::list-of-atoms */
					ZIx = ZIz;
					goto ZL2_list_Hof_Halts_C_Clist_Hof_Hatoms;
					/* END OF INLINE: list-of-alts::list-of-atoms */
				}
				/*UNREACHED*/
			default:
				{
					/* BEGINNING OF ACTION: add-epsilon */
					{
#line 235 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIz), (ZIy))) {
			goto ZL1;
		}
	
#line 358 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 82 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_group_C_Clist_Hof_Hterms(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_grp ZI89)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_grp ZI90;

		p_group_C_Clist_Hof_Hterms_C_Cterm (fsm, cflags, lex_state, act_state, ZI89);
		p_91 (fsm, cflags, lex_state, act_state, ZI89, &ZI90);
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
#line 140 "src/libre/parser.act"

		assert(fsm != NULL);
		/* TODO: assert fsm is empty */

		(ZIx) = fsm_getstart(fsm);
		assert((ZIx) != NULL);

		(ZIy) = fsm_addstate(fsm);
		if ((ZIy) == NULL) {
			goto ZL1;
		}

		fsm_setend(fsm, (ZIy), 1);
	
#line 425 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: make-states */
		/* BEGINNING OF INLINE: 86 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_DOT): case (TOK_OPENSUB): case (TOK_OPENGROUP): case (TOK_ESC):
			case (TOK_CHAR): case (TOK_START): case (TOK_END):
				{
					p_list_Hof_Halts (fsm, cflags, lex_state, act_state, ZIx, ZIy);
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
#line 235 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIy))) {
			goto ZL3;
		}
	
#line 451 "src/libre/form/simple/parser.c"
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
#line 395 "src/libre/parser.act"

		act_state->e = RE_EXALTS;
	
#line 466 "src/libre/form/simple/parser.c"
				}
				/* END OF ACTION: err-expected-alts */
			}
		ZL2:;
		}
		/* END OF INLINE: 86 */
		/* BEGINNING OF INLINE: 87 */
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
#line 399 "src/libre/parser.act"

		act_state->e = RE_EXEOF;
	
#line 493 "src/libre/form/simple/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL4:;
		}
		/* END OF INLINE: 87 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Halts_C_Catom(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state *ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 75 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_DOT):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: add-any */
					{
#line 252 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((*ZIy) != NULL);

		if (!fsm_addedge_any(fsm, (ZIx), (*ZIy))) {
			goto ZL1;
		}
	
#line 531 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: add-any */
				}
				break;
			case (TOK_OPENSUB):
				{
					ADVANCE_LEXER;
					p_list_Hof_Halts (fsm, cflags, lex_state, act_state, ZIx, *ZIy);
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
					t_char ZIc;

					/* BEGINNING OF INLINE: literal */
					{
						switch (CURRENT_TERMINAL) {
						case (TOK_CHAR):
							{
								/* BEGINNING OF EXTRACT: CHAR */
								{
#line 110 "src/libre/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIc = lex_state->buf.a[0];
	
#line 579 "src/libre/form/simple/parser.c"
								}
								/* END OF EXTRACT: CHAR */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_ESC):
							{
								/* BEGINNING OF EXTRACT: ESC */
								{
#line 94 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZIc = lex_state->buf.a[1];

		switch (ZIc) {
		case 't': ZIc = '\t'; break;
		case 'n': ZIc = '\n'; break;
		case 'v': ZIc = '\v'; break;
		case 'r': ZIc = '\r'; break;
		case 'f': ZIc = '\f'; break;
		default:             break;
		}
	
#line 606 "src/libre/form/simple/parser.c"
								}
								/* END OF EXTRACT: ESC */
								ADVANCE_LEXER;
							}
							break;
						default:
							goto ZL1;
						}
					}
					/* END OF INLINE: literal */
					/* BEGINNING OF ACTION: add-literal */
					{
#line 241 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((*ZIy) != NULL);

		/* TODO: check c is legal? */

		if (!fsm_addedge_literal(fsm, (ZIx), (*ZIy), (ZIc))) {
			goto ZL1;
		}
	
#line 630 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: add-literal */
				}
				break;
			case (TOK_START): case (TOK_END):
				{
					t_char ZIc;

					/* BEGINNING OF INLINE: predicate */
					{
						switch (CURRENT_TERMINAL) {
						case (TOK_END):
							{
								/* BEGINNING OF EXTRACT: END */
								{
#line 118 "src/libre/parser.act"

		ZIc = 0;	/* TODO */
	
#line 650 "src/libre/form/simple/parser.c"
								}
								/* END OF EXTRACT: END */
								ADVANCE_LEXER;
							}
							break;
						case (TOK_START):
							{
								/* BEGINNING OF EXTRACT: START */
								{
#line 114 "src/libre/parser.act"

		ZIc = 1;	/* TODO */
	
#line 664 "src/libre/form/simple/parser.c"
								}
								/* END OF EXTRACT: START */
								ADVANCE_LEXER;
							}
							break;
						default:
							goto ZL1;
						}
					}
					/* END OF INLINE: predicate */
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 75 */
		/* BEGINNING OF INLINE: 76 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_OPENCOUNT):
				{
					t_unsigned ZI103;

					ADVANCE_LEXER;
					switch (CURRENT_TERMINAL) {
					case (TOK_COUNT):
						/* BEGINNING OF EXTRACT: COUNT */
						{
#line 122 "src/libre/parser.act"

		ZI103 = strtoul(lex_state->buf.a, NULL, 10);
		/* TODO: range check */
	
#line 699 "src/libre/form/simple/parser.c"
						}
						/* END OF EXTRACT: COUNT */
						break;
					default:
						goto ZL6;
					}
					ADVANCE_LEXER;
					p_104 (fsm, cflags, lex_state, act_state, &ZIx, ZIy, &ZI103);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL6;
					}
				}
				break;
			case (TOK_PLUS):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-1-or-many */
					{
#line 353 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (*ZIy), (ZIx))) {
			goto ZL6;
		}

		/* isolation guard */
		/* TODO: centralise */
		{
			struct fsm_state *z;

			z = fsm_addstate(fsm);
			if (z == NULL) {
				goto ZL6;
			}

			if (!fsm_addedge_epsilon(fsm, (*ZIy), z)) {
				goto ZL6;
			}

			(*ZIy) = z;
		}
	
#line 742 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: count-1-or-many */
				}
				break;
			case (TOK_QMARK):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-0-or-1 */
					{
#line 320 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (*ZIy))) {
			goto ZL6;
		}
	
#line 758 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: count-0-or-1 */
				}
				break;
			case (TOK_STAR):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-0-or-many */
					{
#line 326 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (*ZIy))) {
			goto ZL6;
		}

		if (!fsm_addedge_epsilon(fsm, (*ZIy), (ZIx))) {
			goto ZL6;
		}

		/* isolation guard */
		/* TODO: centralise */
		{
			struct fsm_state *z;

			z = fsm_addstate(fsm);
			if (z == NULL) {
				goto ZL6;
			}

			if (!fsm_addedge_epsilon(fsm, (*ZIy), z)) {
				goto ZL6;
			}

			(*ZIy) = z;
		}
	
#line 795 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: count-0-or-many */
				}
				break;
			default:
				{
					/* BEGINNING OF ACTION: count-1 */
					{
#line 376 "src/libre/parser.act"

		(void) (ZIx);
		(void) (*ZIy);
	
#line 809 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: count-1 */
				}
				break;
			}
			goto ZL5;
		ZL6:;
			{
				/* BEGINNING OF ACTION: err-expected-count */
				{
#line 387 "src/libre/parser.act"

		act_state->e = RE_EXCOUNT;
	
#line 824 "src/libre/form/simple/parser.c"
				}
				/* END OF ACTION: err-expected-count */
			}
		ZL5:;
		}
		/* END OF INLINE: 76 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_91(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_grp ZI89, t_grp *ZO90)
{
	t_grp ZI90;

ZL2_91:;
	switch (CURRENT_TERMINAL) {
	case (TOK_ESC): case (TOK_CHAR):
		{
			p_group_C_Clist_Hof_Hterms_C_Cterm (fsm, cflags, lex_state, act_state, ZI89);
			/* BEGINNING OF INLINE: 91 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_91;
			}
			/* END OF INLINE: 91 */
		}
		/*UNREACHED*/
	default:
		{
			ZI90 = ZI89;
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
	*ZO90 = ZI90;
}

static void
p_list_Hof_Halts_C_Calt(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_list_Hof_Halts_C_Clist_Hof_Hatoms (fsm, cflags, lex_state, act_state, ZIx, ZIy);
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
p_97(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZI93, t_fsm__state ZI94, t_fsm__state *ZO95, t_fsm__state *ZO96)
{
	t_fsm__state ZI95;
	t_fsm__state ZI96;

ZL2_97:;
	switch (CURRENT_TERMINAL) {
	case (TOK_ALT):
		{
			ADVANCE_LEXER;
			p_list_Hof_Halts_C_Calt (fsm, cflags, lex_state, act_state, ZI93, ZI94);
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
			ZI95 = ZI93;
			ZI96 = ZI94;
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
	*ZO95 = ZI95;
	*ZO96 = ZI96;
}

static void
p_100(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_grp *ZIg)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZIb;

			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 82 "src/libre/parser.act"

		ZIb = '-';
	
#line 947 "src/libre/form/simple/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: group-add-char */
			{
#line 176 "src/libre/parser.act"

		assert((*ZIg) != NULL);

		(*ZIg)[(unsigned char) (ZIb)] = 1;
	
#line 959 "src/libre/form/simple/parser.c"
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
p_102(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_grp *ZIg, t_char *ZI101)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZI59;
			t_char ZIb;

			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 82 "src/libre/parser.act"

		ZI59 = '-';
	
#line 986 "src/libre/form/simple/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			/* BEGINNING OF INLINE: 60 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_CHAR):
					{
						/* BEGINNING OF EXTRACT: CHAR */
						{
#line 110 "src/libre/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIb = lex_state->buf.a[0];
	
#line 1004 "src/libre/form/simple/parser.c"
						}
						/* END OF EXTRACT: CHAR */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_RANGE):
					{
						/* BEGINNING OF EXTRACT: RANGE */
						{
#line 82 "src/libre/parser.act"

		ZIb = '-';
	
#line 1018 "src/libre/form/simple/parser.c"
						}
						/* END OF EXTRACT: RANGE */
						ADVANCE_LEXER;
					}
					break;
				default:
					goto ZL1;
				}
			}
			/* END OF INLINE: 60 */
			/* BEGINNING OF ACTION: group-add-range */
			{
#line 184 "src/libre/parser.act"

		int i;

		assert((*ZIg) != NULL);

		for (i = (unsigned char) (*ZI101); i <= (unsigned char) (ZIb); i++) {
			(*ZIg)[i] = 1;
		}
	
#line 1041 "src/libre/form/simple/parser.c"
			}
			/* END OF ACTION: group-add-range */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: group-add-char */
			{
#line 176 "src/libre/parser.act"

		assert((*ZIg) != NULL);

		(*ZIg)[(unsigned char) (*ZI101)] = 1;
	
#line 1056 "src/libre/form/simple/parser.c"
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
p_104(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state *ZIx, t_fsm__state *ZIy, t_unsigned *ZI103)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_CLOSECOUNT):
		{
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: count-m-to-n */
			{
#line 286 "src/libre/parser.act"

		unsigned i;
		struct fsm_state *a;
		struct fsm_state *b;

		if ((*ZI103) == 0) {
			goto ZL1;
		}

		if ((*ZI103) < (*ZI103)) {
			goto ZL1;
		}

		b = (*ZIy);

		for (i = 1; i < (*ZI103); i++) {
			a = fsm_state_duplicatesubgraphx(fsm, (*ZIx), &b);
			if (a == NULL) {
				goto ZL1;
			}

			/* TODO: could elide this epsilon if fsm_state_duplicatesubgraphx()
			 * took an extra parameter giving it a m->new for the start state */
			if (!fsm_addedge_epsilon(fsm, (*ZIy), a)) {
				goto ZL1;
			}

			if (i >= (*ZI103)) {
				if (!fsm_addedge_epsilon(fsm, (*ZIy), b)) {
					goto ZL1;
				}
			}

			(*ZIy) = b;
			(*ZIx) = a;
		}
	
#line 1117 "src/libre/form/simple/parser.c"
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
#line 122 "src/libre/parser.act"

		ZIn = strtoul(lex_state->buf.a, NULL, 10);
		/* TODO: range check */
	
#line 1136 "src/libre/form/simple/parser.c"
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
#line 286 "src/libre/parser.act"

		unsigned i;
		struct fsm_state *a;
		struct fsm_state *b;

		if ((*ZI103) == 0) {
			goto ZL1;
		}

		if ((ZIn) < (*ZI103)) {
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

			if (i >= (*ZI103)) {
				if (!fsm_addedge_epsilon(fsm, (*ZIy), b)) {
					goto ZL1;
				}
			}

			(*ZIy) = b;
			(*ZIx) = a;
		}
	
#line 1191 "src/libre/form/simple/parser.c"
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
p_group_C_Clist_Hof_Hterms_C_Cterm(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_grp ZIg)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_CHAR):
		{
			t_char ZI101;

			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 110 "src/libre/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZI101 = lex_state->buf.a[0];
	
#line 1224 "src/libre/form/simple/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			ADVANCE_LEXER;
			p_102 (fsm, cflags, lex_state, act_state, &ZIg, &ZI101);
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
#line 94 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZIc = lex_state->buf.a[1];

		switch (ZIc) {
		case 't': ZIc = '\t'; break;
		case 'n': ZIc = '\n'; break;
		case 'v': ZIc = '\v'; break;
		case 'r': ZIc = '\r'; break;
		case 'f': ZIc = '\f'; break;
		default:             break;
		}
	
#line 1258 "src/libre/form/simple/parser.c"
			}
			/* END OF EXTRACT: ESC */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: group-add-char */
			{
#line 176 "src/libre/parser.act"

		assert((ZIg) != NULL);

		(ZIg)[(unsigned char) (ZIc)] = 1;
	
#line 1270 "src/libre/form/simple/parser.c"
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
#line 383 "src/libre/parser.act"

		act_state->e = RE_EXTERM;
	
#line 1289 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: err-expected-term */
	}
}

/* BEGINNING OF TRAILER */

#line 403 "src/libre/parser.act"


#line 1300 "src/libre/form/simple/parser.c"

/* END OF FILE */
