/*
 * Automatically generated from the files:
 *	src/libre/form/simple/parser.sid
 * and
 *	src/libre/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 80 "src/libre/parser.act"


	#include <assert.h>
	#include <limits.h>
	#include <string.h>
	#include <stdlib.h>
	#include <errno.h>

	#include <fsm/fsm.h>

	#include <re/re.h>

	#ifndef FORM
	#error FORM required
	#endif

	#define PASTE(a, b) a ## b
	#define CAT(a, b)   PASTE(a, b)

	#define LX_PREFIX CAT(lx_, FORM)

	#define LX_TOKEN  CAT(LX_PREFIX, _token)
	#define LX_STATE  CAT(LX_PREFIX, _lx)
	#define LX_NEXT   CAT(LX_PREFIX, _next)
	#define LX_INIT   CAT(LX_PREFIX, _init)

	#define FORM_COMP CAT(comp_, FORM)

	/* XXX: get rid of this; use same %entry% for all grammars */
	#define FORM_ENTRY CAT(p_re__, FORM)

	#include "parser.h"
	#include "lexer.h"

	#include "../comp.h"

	typedef char     t_char;
	typedef unsigned t_unsigned;
	typedef char *   t_grp;
	typedef unsigned t_pred; /* TODO */

	typedef struct fsm_state * t_fsm__state;

	struct act_state {
		enum LX_TOKEN lex_tok;
		enum LX_TOKEN lex_tok_save;
		enum re_errno e;
	};

	struct lex_state {
		struct LX_STATE lx;
		struct lx_dynbuf buf; /* XXX: unneccessary since we're lexing from a string */

		int (*f)(void *opaque);
		void *opaque;

		/* TODO: use lx's generated conveniences for the pattern buffer */
		char a[512];
		char *p;
	};

	#define CURRENT_TERMINAL (act_state->lex_tok)
	#define ERROR_TERMINAL   (TOK_ERROR)
	#define ADVANCE_LEXER    do { act_state->lex_tok = LX_NEXT(&lex_state->lx); } while (0)
	#define SAVE_LEXER(tok)  do { act_state->lex_tok_save = act_state->lex_tok; \
	                              act_state->lex_tok = tok;                     } while (0)
	#define RESTORE_LEXER    do { act_state->lex_tok = act_state->lex_tok_save; } while (0)

#line 82 "src/libre/form/simple/parser.c"


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
static void p_104(fsm, cflags, lex_state, act_state, t_grp, t_grp *);
static void p_expr_C_Catom(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state *);
static void p_110(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state, t_fsm__state *, t_fsm__state *);
static void p_113(fsm, cflags, lex_state, act_state, t_grp *);
static void p_literal(fsm, cflags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_group_C_Clist_Hof_Hterms_C_Cterm(fsm, cflags, lex_state, act_state, t_grp);
static void p_119(fsm, cflags, lex_state, act_state, t_grp *, t_char *);
static void p_121(fsm, cflags, lex_state, act_state, t_fsm__state *, t_fsm__state *, t_unsigned *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_anchor(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_pred ZIp;

		/* BEGINNING OF INLINE: 80 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_END):
				{
					/* BEGINNING OF EXTRACT: END */
					{
#line 200 "src/libre/parser.act"

		switch (cflags & RE_ANCHOR) {
		case RE_TEXT:  ZIp = RE_EOT; break;
		case RE_MULTI: ZIp = RE_EOL; break;
		case RE_ZONE:  ZIp = RE_EOZ; break;

		default:
			/* TODO: raise error */
			ZIp = 0U;
		}
	
#line 142 "src/libre/form/simple/parser.c"
					}
					/* END OF EXTRACT: END */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_START):
				{
					/* BEGINNING OF EXTRACT: START */
					{
#line 188 "src/libre/parser.act"

		switch (cflags & RE_ANCHOR) {
		case RE_TEXT:  ZIp = RE_SOT; break;
		case RE_MULTI: ZIp = RE_SOL; break;
		case RE_ZONE:  ZIp = RE_SOZ; break;

		default:
			/* TODO: raise error */
			ZIp = 0U;
		}
	
#line 164 "src/libre/form/simple/parser.c"
					}
					/* END OF EXTRACT: START */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 80 */
		/* BEGINNING OF ACTION: add-pred */
		{
#line 330 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

/* TODO:
		if (!fsm_addedge_predicate(fsm, (ZIx), (ZIy), (ZIp))) {
			goto ZL1;
		}
*/
	
#line 188 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: add-pred */
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
#line 317 "src/libre/parser.act"

		(ZIz) = fsm_addstate(fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 216 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: add-concat */
		/* BEGINNING OF ACTION: add-epsilon */
		{
#line 324 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIz))) {
			goto ZL1;
		}
	
#line 227 "src/libre/form/simple/parser.c"
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
		t_char ZI76;

		switch (CURRENT_TERMINAL) {
		case (TOK_OPENGROUP):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: make-group */
		{
#line 242 "src/libre/parser.act"

		(ZIg) = calloc(UCHAR_MAX + 1, sizeof *(ZIg));
		if ((ZIg) == NULL) {
			goto ZL1;
		}
	
#line 268 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: make-group */
		/* BEGINNING OF INLINE: 74 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_INVERT):
				{
					t_char ZI75;

					/* BEGINNING OF EXTRACT: INVERT */
					{
#line 101 "src/libre/parser.act"

		ZI75 = '^';
	
#line 284 "src/libre/form/simple/parser.c"
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
#line 257 "src/libre/parser.act"

		int i;

		assert((ZIg) != NULL);

		for (i = 0; i <= UCHAR_MAX; i++) {
			(ZIg)[i] = !(ZIg)[i];
		}
	
#line 305 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: invert-group */
				}
				break;
			case (TOK_CLOSEGROUP): case (TOK_RANGE): case (TOK_ESC): case (TOK_OCT):
			case (TOK_HEX): case (TOK_CHAR):
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
		/* END OF INLINE: 74 */
		/* BEGINNING OF ACTION: group-to-states */
		{
#line 283 "src/libre/parser.act"

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
	
#line 344 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: group-to-states */
		/* BEGINNING OF ACTION: free-group */
		{
#line 249 "src/libre/parser.act"

		assert((ZIg) != NULL);

		free((ZIg));
	
#line 355 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: free-group */
		switch (CURRENT_TERMINAL) {
		case (TOK_CLOSEGROUP):
			/* BEGINNING OF EXTRACT: CLOSEGROUP */
			{
#line 109 "src/libre/parser.act"

		ZI76 = ']';
	
#line 366 "src/libre/form/simple/parser.c"
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
		/* BEGINNING OF INLINE: 72 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSEGROUP):
				{
					t_char ZI112;

					/* BEGINNING OF EXTRACT: CLOSEGROUP */
					{
#line 109 "src/libre/parser.act"

		ZI112 = ']';
	
#line 401 "src/libre/form/simple/parser.c"
					}
					/* END OF EXTRACT: CLOSEGROUP */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: group-add-char */
					{
#line 265 "src/libre/parser.act"

		assert((ZIg) != NULL);

		(ZIg)[(unsigned char) (ZI112)] = 1;
	
#line 413 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: group-add-char */
					p_113 (fsm, cflags, lex_state, act_state, &ZIg);
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
#line 105 "src/libre/parser.act"

		ZIc = '-';
	
#line 433 "src/libre/form/simple/parser.c"
					}
					/* END OF EXTRACT: RANGE */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: group-add-char */
					{
#line 265 "src/libre/parser.act"

		assert((ZIg) != NULL);

		(ZIg)[(unsigned char) (ZIc)] = 1;
	
#line 445 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: group-add-char */
				}
				break;
			default:
				break;
			}
		}
		/* END OF INLINE: 72 */
		p_group_C_Clist_Hof_Hterms (fsm, cflags, lex_state, act_state, ZIg);
		/* BEGINNING OF INLINE: 73 */
		{
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
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
p_group_C_Clist_Hof_Hterms(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_grp ZI102)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_grp ZI103;

		p_group_C_Clist_Hof_Hterms_C_Cterm (fsm, cflags, lex_state, act_state, ZI102);
		p_104 (fsm, cflags, lex_state, act_state, ZI102, &ZI103);
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
#line 229 "src/libre/parser.act"

		assert(fsm != NULL);
		/* TODO: assert fsm is empty */

		(ZIx) = fsm_getstart(fsm);
		assert((ZIx) != NULL);

		(ZIy) = fsm_addstate(fsm);
		if ((ZIy) == NULL) {
			goto ZL1;
		}

		fsm_setend(fsm, (ZIy), 1);
	
#line 539 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: make-states */
		/* BEGINNING OF INLINE: 99 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_DOT): case (TOK_OPENSUB): case (TOK_OPENGROUP): case (TOK_ESC):
			case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR): case (TOK_START):
			case (TOK_END):
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
#line 324 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIy))) {
			goto ZL3;
		}
	
#line 566 "src/libre/form/simple/parser.c"
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
#line 495 "src/libre/parser.act"

		act_state->e = RE_EXALTS;
	
#line 581 "src/libre/form/simple/parser.c"
				}
				/* END OF ACTION: err-expected-alts */
			}
		ZL2:;
		}
		/* END OF INLINE: 99 */
		/* BEGINNING OF INLINE: 100 */
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
#line 503 "src/libre/parser.act"

		act_state->e = RE_EXEOF;
	
#line 608 "src/libre/form/simple/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL4:;
		}
		/* END OF INLINE: 100 */
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
#line 317 "src/libre/parser.act"

		(ZIz) = fsm_addstate(fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 641 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: add-concat */
		/* BEGINNING OF INLINE: 92 */
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
			case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
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
		/* END OF INLINE: 92 */
		/* BEGINNING OF INLINE: 93 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_DOT): case (TOK_OPENSUB): case (TOK_OPENGROUP): case (TOK_ESC):
			case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR): case (TOK_START):
			case (TOK_END):
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
#line 324 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIz), (ZIy))) {
			goto ZL1;
		}
	
#line 694 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			}
		}
		/* END OF INLINE: 93 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_expr_C_Clist_Hof_Halts(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZI106, t_fsm__state ZI107)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_fsm__state ZI108;
		t_fsm__state ZI109;

		p_expr_C_Clist_Hof_Halts_C_Calt (fsm, cflags, lex_state, act_state, ZI106, ZI107);
		p_110 (fsm, cflags, lex_state, act_state, ZI106, ZI107, &ZI108, &ZI109);
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
p_104(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_grp ZI102, t_grp *ZO103)
{
	t_grp ZI103;

ZL2_104:;
	switch (CURRENT_TERMINAL) {
	case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
		{
			p_group_C_Clist_Hof_Hterms_C_Cterm (fsm, cflags, lex_state, act_state, ZI102);
			/* BEGINNING OF INLINE: 104 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_104;
			}
			/* END OF INLINE: 104 */
		}
		/*UNREACHED*/
	default:
		{
			ZI103 = ZI102;
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
	*ZO103 = ZI103;
}

static void
p_expr_C_Catom(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state *ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 85 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_DOT):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: add-any */
					{
#line 352 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((*ZIy) != NULL);

		if (!fsm_addedge_any(fsm, (ZIx), (*ZIy))) {
			goto ZL1;
		}
	
#line 792 "src/libre/form/simple/parser.c"
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
			case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
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
		/* END OF INLINE: 85 */
		/* BEGINNING OF INLINE: 86 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_OPENCOUNT):
				{
					t_unsigned ZI120;

					ADVANCE_LEXER;
					switch (CURRENT_TERMINAL) {
					case (TOK_COUNT):
						/* BEGINNING OF EXTRACT: COUNT */
						{
#line 211 "src/libre/parser.act"

		ZI120 = strtoul(lex_state->buf.a, NULL, 10);
		/* TODO: range check */
	
#line 853 "src/libre/form/simple/parser.c"
						}
						/* END OF EXTRACT: COUNT */
						break;
					default:
						goto ZL4;
					}
					ADVANCE_LEXER;
					p_121 (fsm, cflags, lex_state, act_state, &ZIx, ZIy, &ZI120);
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
#line 453 "src/libre/parser.act"

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
	
#line 896 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: count-1-or-many */
				}
				break;
			case (TOK_QMARK):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-0-or-1 */
					{
#line 420 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (*ZIy))) {
			goto ZL4;
		}
	
#line 912 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: count-0-or-1 */
				}
				break;
			case (TOK_STAR):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: count-0-or-many */
					{
#line 426 "src/libre/parser.act"

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
	
#line 949 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: count-0-or-many */
				}
				break;
			default:
				{
					/* BEGINNING OF ACTION: count-1 */
					{
#line 476 "src/libre/parser.act"

		(void) (ZIx);
		(void) (*ZIy);
	
#line 963 "src/libre/form/simple/parser.c"
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
#line 487 "src/libre/parser.act"

		act_state->e = RE_EXCOUNT;
	
#line 978 "src/libre/form/simple/parser.c"
				}
				/* END OF ACTION: err-expected-count */
			}
		ZL3:;
		}
		/* END OF INLINE: 86 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_110(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZI106, t_fsm__state ZI107, t_fsm__state *ZO108, t_fsm__state *ZO109)
{
	t_fsm__state ZI108;
	t_fsm__state ZI109;

ZL2_110:;
	switch (CURRENT_TERMINAL) {
	case (TOK_ALT):
		{
			ADVANCE_LEXER;
			p_expr_C_Clist_Hof_Halts_C_Calt (fsm, cflags, lex_state, act_state, ZI106, ZI107);
			/* BEGINNING OF INLINE: 110 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_110;
			}
			/* END OF INLINE: 110 */
		}
		/*UNREACHED*/
	default:
		{
			ZI108 = ZI106;
			ZI109 = ZI107;
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
	*ZO108 = ZI108;
	*ZO109 = ZI109;
}

static void
p_113(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_grp *ZIg)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZIb;

			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 105 "src/libre/parser.act"

		ZIb = '-';
	
#line 1046 "src/libre/form/simple/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: group-add-char */
			{
#line 265 "src/libre/parser.act"

		assert((*ZIg) != NULL);

		(*ZIg)[(unsigned char) (ZIb)] = 1;
	
#line 1058 "src/libre/form/simple/parser.c"
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
p_literal(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZIc;

		/* BEGINNING OF INLINE: 78 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 183 "src/libre/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIc = lex_state->buf.a[0];
	
#line 1093 "src/libre/form/simple/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_ESC):
				{
					/* BEGINNING OF EXTRACT: ESC */
					{
#line 117 "src/libre/parser.act"

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
	
#line 1120 "src/libre/form/simple/parser.c"
					}
					/* END OF EXTRACT: ESC */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_HEX):
				{
					/* BEGINNING OF EXTRACT: HEX */
					{
#line 176 "src/libre/parser.act"

		unsigned long u;
		char *e;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 3);

		errno = 0;

		u = strtoul(lex_state->buf.a + 2, &e, 16);
		assert(*e == '\0');

		if (u > UCHAR_MAX || (u == ULONG_MAX && errno == ERANGE)) {
			/* TODO: range check */
		}

		if (u == ULONG_MAX && errno != 0) {
			/* TODO: range check */
		}

		ZIc = (unsigned char) u;
	
#line 1153 "src/libre/form/simple/parser.c"
					}
					/* END OF EXTRACT: HEX */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_OCT):
				{
					/* BEGINNING OF EXTRACT: OCT */
					{
#line 153 "src/libre/parser.act"

		unsigned long u;
		char *e;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		errno = 0;

		u = strtoul(lex_state->buf.a + 1, &e, 8);
		assert(*e == '\0');

		if (u > UCHAR_MAX || (u == ULONG_MAX && errno == ERANGE)) {
			err(lex_state, "octal escape %s out of range: expected \\0..\\%o inclusive",
				lex_state->buf.a, UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		if (u == ULONG_MAX && errno != 0) {
			err(lex_state, "%s: %s: expected \\0..\\%o inclusive",
				lex_state->buf.a, strerror(errno), UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		ZIc = (unsigned char) u;
	
#line 1190 "src/libre/form/simple/parser.c"
					}
					/* END OF EXTRACT: OCT */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 78 */
		/* BEGINNING OF ACTION: add-literal */
		{
#line 341 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		/* TODO: check c is legal? */

		if (!fsm_addedge_literal(fsm, (ZIx), (ZIy), (ZIc))) {
			goto ZL1;
		}
	
#line 1214 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: add-literal */
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
			t_char ZI118;

			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 183 "src/libre/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZI118 = lex_state->buf.a[0];
	
#line 1241 "src/libre/form/simple/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			ADVANCE_LEXER;
			p_119 (fsm, cflags, lex_state, act_state, &ZIg, &ZI118);
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
#line 117 "src/libre/parser.act"

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
	
#line 1275 "src/libre/form/simple/parser.c"
			}
			/* END OF EXTRACT: ESC */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: group-add-char */
			{
#line 265 "src/libre/parser.act"

		assert((ZIg) != NULL);

		(ZIg)[(unsigned char) (ZIc)] = 1;
	
#line 1287 "src/libre/form/simple/parser.c"
			}
			/* END OF ACTION: group-add-char */
		}
		break;
	case (TOK_HEX):
		{
			t_char ZI116;

			/* BEGINNING OF EXTRACT: HEX */
			{
#line 176 "src/libre/parser.act"

		unsigned long u;
		char *e;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 3);

		errno = 0;

		u = strtoul(lex_state->buf.a + 2, &e, 16);
		assert(*e == '\0');

		if (u > UCHAR_MAX || (u == ULONG_MAX && errno == ERANGE)) {
			/* TODO: range check */
		}

		if (u == ULONG_MAX && errno != 0) {
			/* TODO: range check */
		}

		ZI116 = (unsigned char) u;
	
#line 1321 "src/libre/form/simple/parser.c"
			}
			/* END OF EXTRACT: HEX */
			ADVANCE_LEXER;
			p_119 (fsm, cflags, lex_state, act_state, &ZIg, &ZI116);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_OCT):
		{
			t_char ZI114;

			/* BEGINNING OF EXTRACT: OCT */
			{
#line 153 "src/libre/parser.act"

		unsigned long u;
		char *e;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		errno = 0;

		u = strtoul(lex_state->buf.a + 1, &e, 8);
		assert(*e == '\0');

		if (u > UCHAR_MAX || (u == ULONG_MAX && errno == ERANGE)) {
			err(lex_state, "octal escape %s out of range: expected \\0..\\%o inclusive",
				lex_state->buf.a, UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		if (u == ULONG_MAX && errno != 0) {
			err(lex_state, "%s: %s: expected \\0..\\%o inclusive",
				lex_state->buf.a, strerror(errno), UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		ZI114 = (unsigned char) u;
	
#line 1365 "src/libre/form/simple/parser.c"
			}
			/* END OF EXTRACT: OCT */
			ADVANCE_LEXER;
			p_119 (fsm, cflags, lex_state, act_state, &ZIg, &ZI114);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
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
#line 483 "src/libre/parser.act"

		act_state->e = RE_EXTERM;
	
#line 1390 "src/libre/form/simple/parser.c"
		}
		/* END OF ACTION: err-expected-term */
	}
}

static void
p_119(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_grp *ZIg, t_char *ZI118)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZIb;

			/* BEGINNING OF INLINE: 65 */
			{
				{
					t_char ZI66;

					switch (CURRENT_TERMINAL) {
					case (TOK_RANGE):
						/* BEGINNING OF EXTRACT: RANGE */
						{
#line 105 "src/libre/parser.act"

		ZI66 = '-';
	
#line 1417 "src/libre/form/simple/parser.c"
						}
						/* END OF EXTRACT: RANGE */
						break;
					default:
						goto ZL3;
					}
					ADVANCE_LEXER;
				}
				goto ZL2;
			ZL3:;
				{
					/* BEGINNING OF ACTION: err-expected-range */
					{
#line 499 "src/libre/parser.act"

		act_state->e = RE_EXRANGE;
	
#line 1435 "src/libre/form/simple/parser.c"
					}
					/* END OF ACTION: err-expected-range */
				}
			ZL2:;
			}
			/* END OF INLINE: 65 */
			/* BEGINNING OF INLINE: 67 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_CHAR):
					{
						/* BEGINNING OF EXTRACT: CHAR */
						{
#line 183 "src/libre/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIb = lex_state->buf.a[0];
	
#line 1456 "src/libre/form/simple/parser.c"
						}
						/* END OF EXTRACT: CHAR */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_HEX):
					{
						/* BEGINNING OF EXTRACT: HEX */
						{
#line 176 "src/libre/parser.act"

		unsigned long u;
		char *e;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 3);

		errno = 0;

		u = strtoul(lex_state->buf.a + 2, &e, 16);
		assert(*e == '\0');

		if (u > UCHAR_MAX || (u == ULONG_MAX && errno == ERANGE)) {
			/* TODO: range check */
		}

		if (u == ULONG_MAX && errno != 0) {
			/* TODO: range check */
		}

		ZIb = (unsigned char) u;
	
#line 1489 "src/libre/form/simple/parser.c"
						}
						/* END OF EXTRACT: HEX */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_OCT):
					{
						/* BEGINNING OF EXTRACT: OCT */
						{
#line 153 "src/libre/parser.act"

		unsigned long u;
		char *e;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		errno = 0;

		u = strtoul(lex_state->buf.a + 1, &e, 8);
		assert(*e == '\0');

		if (u > UCHAR_MAX || (u == ULONG_MAX && errno == ERANGE)) {
			err(lex_state, "octal escape %s out of range: expected \\0..\\%o inclusive",
				lex_state->buf.a, UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		if (u == ULONG_MAX && errno != 0) {
			err(lex_state, "%s: %s: expected \\0..\\%o inclusive",
				lex_state->buf.a, strerror(errno), UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		ZIb = (unsigned char) u;
	
#line 1526 "src/libre/form/simple/parser.c"
						}
						/* END OF EXTRACT: OCT */
						ADVANCE_LEXER;
					}
					break;
				case (TOK_RANGE):
					{
						/* BEGINNING OF EXTRACT: RANGE */
						{
#line 105 "src/libre/parser.act"

		ZIb = '-';
	
#line 1540 "src/libre/form/simple/parser.c"
						}
						/* END OF EXTRACT: RANGE */
						ADVANCE_LEXER;
					}
					break;
				default:
					goto ZL1;
				}
			}
			/* END OF INLINE: 67 */
			/* BEGINNING OF ACTION: group-add-range */
			{
#line 273 "src/libre/parser.act"

		int i;

		assert((*ZIg) != NULL);

		for (i = (unsigned char) (*ZI118); i <= (unsigned char) (ZIb); i++) {
			(*ZIg)[i] = 1;
		}
	
#line 1563 "src/libre/form/simple/parser.c"
			}
			/* END OF ACTION: group-add-range */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: group-add-char */
			{
#line 265 "src/libre/parser.act"

		assert((*ZIg) != NULL);

		(*ZIg)[(unsigned char) (*ZI118)] = 1;
	
#line 1578 "src/libre/form/simple/parser.c"
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
p_121(fsm fsm, cflags cflags, lex_state lex_state, act_state act_state, t_fsm__state *ZIx, t_fsm__state *ZIy, t_unsigned *ZI120)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_CLOSECOUNT):
		{
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: count-m-to-n */
			{
#line 386 "src/libre/parser.act"

		unsigned i;
		struct fsm_state *a;
		struct fsm_state *b;

		if ((*ZI120) == 0) {
			goto ZL1;
		}

		if ((*ZI120) < (*ZI120)) {
			goto ZL1;
		}

		b = (*ZIy);

		for (i = 1; i < (*ZI120); i++) {
			a = fsm_state_duplicatesubgraphx(fsm, (*ZIx), &b);
			if (a == NULL) {
				goto ZL1;
			}

			/* TODO: could elide this epsilon if fsm_state_duplicatesubgraphx()
			 * took an extra parameter giving it a m->new for the start state */
			if (!fsm_addedge_epsilon(fsm, (*ZIy), a)) {
				goto ZL1;
			}

			if (i >= (*ZI120)) {
				if (!fsm_addedge_epsilon(fsm, (*ZIy), b)) {
					goto ZL1;
				}
			}

			(*ZIy) = b;
			(*ZIx) = a;
		}
	
#line 1639 "src/libre/form/simple/parser.c"
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
#line 211 "src/libre/parser.act"

		ZIn = strtoul(lex_state->buf.a, NULL, 10);
		/* TODO: range check */
	
#line 1658 "src/libre/form/simple/parser.c"
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
#line 386 "src/libre/parser.act"

		unsigned i;
		struct fsm_state *a;
		struct fsm_state *b;

		if ((*ZI120) == 0) {
			goto ZL1;
		}

		if ((ZIn) < (*ZI120)) {
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

			if (i >= (*ZI120)) {
				if (!fsm_addedge_epsilon(fsm, (*ZIy), b)) {
					goto ZL1;
				}
			}

			(*ZIy) = b;
			(*ZIx) = a;
		}
	
#line 1713 "src/libre/form/simple/parser.c"
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

/* BEGINNING OF TRAILER */

#line 596 "src/libre/parser.act"


	static int
	lgetc(struct LX_STATE *lx)
	{
		const struct lex_state *lex_state;

		assert(lx != NULL);
		assert(lx->opaque != NULL);

		lex_state = lx->opaque;

		assert(lex_state->f != NULL);

		return lex_state->f(lex_state->opaque);
	}

	struct fsm *
	FORM_COMP(int (*f)(void *opaque), void *opaque,
		enum re_cflags cflags, struct re_err *err)
	{
		struct act_state  act_state_s;
		struct act_state *act_state;
		struct lex_state  lex_state_s;
		struct lex_state *lex_state;

		struct LX_STATE *lx;
		struct fsm *new;
		enum re_errno e;

		assert(f != NULL);

		new = re_new_empty();
		if (new == NULL) {
			e = RE_EERRNO;
			goto error;
		}

		lex_state    = &lex_state_s;
		lex_state->p = lex_state->a;

		lx = &lex_state->lx;

		LX_INIT(lx);

		lx->lgetc  = lgetc;
		lx->opaque = lex_state;

		lex_state->f       = f;
		lex_state->opaque  = opaque;

		lex_state->buf.a   = NULL;
		lex_state->buf.len = 0;

		/* XXX: unneccessary since we're lexing from a string */
		lx->buf   = &lex_state->buf;
		lx->push  = CAT(LX_PREFIX, _dynpush);
		lx->pop   = CAT(LX_PREFIX, _dynpop);
		lx->clear = CAT(LX_PREFIX, _dynclear);
		lx->free  = CAT(LX_PREFIX, _dynfree);

	/* XXX */
	lx->free = NULL;

		/* This is a workaround for ADVANCE_LEXER assuming a pointer */
		act_state = &act_state_s;

		act_state->e = RE_ESUCCESS;

		ADVANCE_LEXER;
		FORM_ENTRY(new, cflags, lex_state, act_state);

		if (act_state->e != RE_ESUCCESS) {
			/* TODO: free internals allocated during parsing (are there any?) */
			fsm_free(new);
			e = act_state->e;
			goto error;
		}

		return new;

	error:

		if (err != NULL) {
			err->e    = e;
			err->byte = lx->start.byte;
		}

		return NULL;
	}

#line 1823 "src/libre/form/simple/parser.c"

/* END OF FILE */
