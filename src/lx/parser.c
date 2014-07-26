/*
 * Automatically generated from the files:
 *	parser.sid
 * and
 *	parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 75 "parser.act"


	#include <assert.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <errno.h>

	#include <adt/xalloc.h>

	#include <fsm/fsm.h>
	#include <fsm/bool.h>
	#include <fsm/pred.h>
	#include <fsm/graph.h>

	#include <re/re.h>

	#include "libfsm/internal.h"

	#include "lexer.h"
	#include "parser.h"
	#include "ast.h"
	#include "var.h"

	typedef char *               string;
	typedef struct fsm *         fsm;
	typedef struct ast_zone *    zone;
	typedef struct ast_mapping * mapping;

	struct act_state {
		enum lx_token lex_tok;
		enum lx_token lex_tok_save;
	};

	struct lex_state {
		struct lx lx;
		struct lx_dynbuf buf;

		/* TODO: use lx's generated conveniences for the pattern buffer */
		char a[512];
		char *p;
	};

	#define CURRENT_TERMINAL (act_state->lex_tok)
	#define ERROR_TERMINAL   TOK_UNKNOWN
	#define ADVANCE_LEXER    do { act_state->lex_tok = lx_next(&lex_state->lx); } while (0)
	#define SAVE_LEXER(tok)  do { act_state->lex_tok_save = act_state->lex_tok; \
	                              act_state->lex_tok = tok;                     } while (0)
	#define RESTORE_LEXER    do { act_state->lex_tok = act_state->lex_tok_save; } while (0)

	static void err_expected(const char *token) {
		fprintf(stderr, "syntax error: expected %s\n", token);
		exit(EXIT_FAILURE);
	}

#line 68 "parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_concaternated_Hpatterns_C_Cpattern__regex(lex_state, act_state);
static void p_list_Hof_Hthings(lex_state, act_state, ast, zone, fsm);
static void p_concaternated_Hpatterns_C_Cpattern(lex_state, act_state, fsm *);
static void p_concaternated_Hpatterns(lex_state, act_state, fsm *);
static void p_list_Hof_Hthings_C_Cthing(lex_state, act_state, ast, zone, fsm);
static void p_token_Hmapping(lex_state, act_state, fsm *, string *);
static void p_concaternated_Hpatterns_C_Cpattern__literal(lex_state, act_state);
static void p_94(lex_state, act_state);
static void p_list_Hof_Hthings_C_Cbind_Hthing(lex_state, act_state, ast, zone, fsm);
extern void p_lx(lex_state, act_state, ast *);
static void p_114(lex_state, act_state, ast, zone, fsm, ast *, zone *, fsm *);
static void p_118(lex_state, act_state, fsm, fsm *);
static void p_124(lex_state, act_state, ast *, zone *, string *, fsm *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_concaternated_Hpatterns_C_Cpattern__regex(lex_state lex_state, act_state act_state)
{
ZL2_concaternated_Hpatterns_C_Cpattern__regex:;
	switch (CURRENT_TERMINAL) {
	case (TOK_CHAR):
		{
			char ZIc;

			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 109 "parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIc = lex_state->buf.a[0];
	
#line 114 "parser.c"
			}
			/* END OF EXTRACT: CHAR */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: pattern-char */
			{
#line 133 "parser.act"

		/* TODO */
		*lex_state->p++ = (ZIc);
	
#line 125 "parser.c"
			}
			/* END OF ACTION: pattern-char */
			/* BEGINNING OF INLINE: concaternated-patterns::pattern_regex */
			goto ZL2_concaternated_Hpatterns_C_Cpattern__regex;
			/* END OF INLINE: concaternated-patterns::pattern_regex */
		}
		/*UNREACHED*/
	case (ERROR_TERMINAL):
		return;
	default:
		break;
	}
}

static void
p_list_Hof_Hthings(lex_state lex_state, act_state act_state, ast ZI108, zone ZI109, fsm ZI110)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		ast ZI111;
		zone ZI112;
		fsm ZI113;

		p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI108, ZI109, ZI110);
		p_114 (lex_state, act_state, ZI108, ZI109, ZI110, &ZI111, &ZI112, &ZI113);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-list */
		{
#line 400 "parser.act"

		err_expected("list of token mappings or zones");
	
#line 167 "parser.c"
		}
		/* END OF ACTION: err-expected-list */
	}
}

static void
p_concaternated_Hpatterns_C_Cpattern(lex_state lex_state, act_state act_state, fsm *ZOr)
{
	fsm ZIr;

	switch (CURRENT_TERMINAL) {
	case (TOK_RESTART):
		{
			string ZIs;

			ADVANCE_LEXER;
			p_concaternated_Hpatterns_C_Cpattern__regex (lex_state, act_state);
			switch (CURRENT_TERMINAL) {
			case (TOK_REEND):
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: pattern-buffer */
			{
#line 140 "parser.act"

		/* TODO */
		*lex_state->p++ = '\0';

		(ZIs) = lex_state->a;

		lex_state->p = lex_state->a;
	
#line 206 "parser.c"
			}
			/* END OF ACTION: pattern-buffer */
			/* BEGINNING OF ACTION: compile-regex */
			{
#line 157 "parser.act"

		assert((ZIs) != NULL);

		/* TODO: handle error codes */
		(ZIr) = re_new_comp(RE_SIMPLE, re_getc_str, &(ZIs), 0, NULL);
		if ((ZIr) == NULL) {
			/* TODO: use *err */
			goto ZL1;
		}
	
#line 222 "parser.c"
			}
			/* END OF ACTION: compile-regex */
		}
		break;
	case (TOK_STRSTART):
		{
			string ZIs;

			ADVANCE_LEXER;
			p_concaternated_Hpatterns_C_Cpattern__literal (lex_state, act_state);
			switch (CURRENT_TERMINAL) {
			case (TOK_STREND):
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: pattern-buffer */
			{
#line 140 "parser.act"

		/* TODO */
		*lex_state->p++ = '\0';

		(ZIs) = lex_state->a;

		lex_state->p = lex_state->a;
	
#line 254 "parser.c"
			}
			/* END OF ACTION: pattern-buffer */
			/* BEGINNING OF ACTION: compile-literal */
			{
#line 146 "parser.act"

		assert((ZIs) != NULL);

		/* TODO: handle error codes */
		(ZIr) = re_new_comp(RE_LITERAL, re_getc_str, &(ZIs), 0, NULL);
		if ((ZIr) == NULL) {
			/* TODO: use *err */
			goto ZL1;
		}
	
#line 270 "parser.c"
			}
			/* END OF ACTION: compile-literal */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	default:
		goto ZL1;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOr = ZIr;
}

static void
p_concaternated_Hpatterns(lex_state lex_state, act_state act_state, fsm *ZO117)
{
	fsm ZI117;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIq;

		p_concaternated_Hpatterns_C_Cpattern (lex_state, act_state, &ZIq);
		p_118 (lex_state, act_state, ZIq, &ZI117);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZO117 = ZI117;
}

static void
p_list_Hof_Hthings_C_Cthing(lex_state lex_state, act_state act_state, ast ZIa, zone ZIz, fsm ZIexit)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_IDENT):
		{
			p_list_Hof_Hthings_C_Cbind_Hthing (lex_state, act_state, ZIa, ZIz, ZIexit);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_RESTART): case (TOK_STRSTART):
		{
			fsm ZI121;
			string ZI122;
			fsm ZI123;

			p_token_Hmapping (lex_state, act_state, &ZI121, &ZI122);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 323 "parser.act"

		assert((ZI121) != NULL);

		if ((ZIexit) == NULL) {
			(ZI123) = (ZI121);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI123) = fsm_subtract((ZI121), (ZIexit));
			if ((ZI123) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if (!fsm_trim((ZI123))) {
				perror("fsm_trim");
				goto ZL1;
			}
		}
	
#line 365 "parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_124 (lex_state, act_state, &ZIa, &ZIz, &ZI122, &ZI123);
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
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_token_Hmapping(lex_state lex_state, act_state act_state, fsm *ZOr, string *ZOt)
{
	fsm ZIr;
	string ZIt;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_concaternated_Hpatterns (lex_state, act_state, &ZIr);
		/* BEGINNING OF INLINE: 70 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_MAP):
				{
					/* BEGINNING OF INLINE: 71 */
					{
						{
							switch (CURRENT_TERMINAL) {
							case (TOK_MAP):
								break;
							default:
								goto ZL4;
							}
							ADVANCE_LEXER;
						}
						goto ZL3;
					ZL4:;
						{
							/* BEGINNING OF ACTION: err-expected-map */
							{
#line 372 "parser.act"

		err_expected("'->'");
	
#line 422 "parser.c"
							}
							/* END OF ACTION: err-expected-map */
						}
					ZL3:;
					}
					/* END OF INLINE: 71 */
					switch (CURRENT_TERMINAL) {
					case (TOK_TOKEN):
						/* BEGINNING OF EXTRACT: TOKEN */
						{
#line 114 "parser.act"

		/* TODO: submatch addressing */
		ZIt = xstrdup(lex_state->buf.a + 1); /* +1 for '$' prefix */
		if (ZIt == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 442 "parser.c"
						}
						/* END OF EXTRACT: TOKEN */
						break;
					default:
						goto ZL1;
					}
					ADVANCE_LEXER;
				}
				break;
			default:
				{
					/* BEGINNING OF ACTION: no-token */
					{
#line 271 "parser.act"

		(ZIt) = NULL;
	
#line 460 "parser.c"
					}
					/* END OF ACTION: no-token */
				}
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 70 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOr = ZIr;
	*ZOt = ZIt;
}

static void
p_concaternated_Hpatterns_C_Cpattern__literal(lex_state lex_state, act_state act_state)
{
ZL2_concaternated_Hpatterns_C_Cpattern__literal:;
	switch (CURRENT_TERMINAL) {
	case (TOK_CHAR):
		{
			char ZIc;

			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 109 "parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIc = lex_state->buf.a[0];
	
#line 499 "parser.c"
			}
			/* END OF EXTRACT: CHAR */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: pattern-char */
			{
#line 133 "parser.act"

		/* TODO */
		*lex_state->p++ = (ZIc);
	
#line 510 "parser.c"
			}
			/* END OF ACTION: pattern-char */
			/* BEGINNING OF INLINE: concaternated-patterns::pattern_literal */
			goto ZL2_concaternated_Hpatterns_C_Cpattern__literal;
			/* END OF INLINE: concaternated-patterns::pattern_literal */
		}
		/*UNREACHED*/
	case (TOK_ESC):
		{
			char ZIc;

			/* BEGINNING OF EXTRACT: ESC */
			{
#line 97 "parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		switch (lex_state->buf.a[1]) {
		case 'n':  ZIc = '\n'; break;
		case 't':  ZIc = '\t'; break;
		case 'r':  ZIc = '\r'; break;
		case '\\': ZIc = '\\'; break;
		default:   ZIc = '\0'; break; /* TODO: handle error */
		}
	
#line 538 "parser.c"
			}
			/* END OF EXTRACT: ESC */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: pattern-char */
			{
#line 133 "parser.act"

		/* TODO */
		*lex_state->p++ = (ZIc);
	
#line 549 "parser.c"
			}
			/* END OF ACTION: pattern-char */
			/* BEGINNING OF INLINE: concaternated-patterns::pattern_literal */
			goto ZL2_concaternated_Hpatterns_C_Cpattern__literal;
			/* END OF INLINE: concaternated-patterns::pattern_literal */
		}
		/*UNREACHED*/
	case (ERROR_TERMINAL):
		return;
	default:
		break;
	}
}

static void
p_94(lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case (TOK_SEMI):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-semi */
		{
#line 380 "parser.act"

		err_expected("';'");
	
#line 588 "parser.c"
		}
		/* END OF ACTION: err-expected-semi */
	}
}

static void
p_list_Hof_Hthings_C_Cbind_Hthing(lex_state lex_state, act_state act_state, ast ZIa, zone ZIz, fsm ZIexit)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		string ZIn;
		fsm ZIr;
		fsm ZIq;

		switch (CURRENT_TERMINAL) {
		case (TOK_IDENT):
			/* BEGINNING OF EXTRACT: IDENT */
			{
#line 122 "parser.act"

		ZIn = xstrdup(lex_state->buf.a);
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 617 "parser.c"
			}
			/* END OF EXTRACT: IDENT */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF INLINE: 78 */
		{
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_BIND):
					break;
				default:
					goto ZL3;
				}
				ADVANCE_LEXER;
			}
			goto ZL2;
		ZL3:;
			{
				/* BEGINNING OF ACTION: err-expected-bind */
				{
#line 376 "parser.act"

		err_expected("'='");
	
#line 645 "parser.c"
				}
				/* END OF ACTION: err-expected-bind */
			}
		ZL2:;
		}
		/* END OF INLINE: 78 */
		p_concaternated_Hpatterns (lex_state, act_state, &ZIr);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
		/* BEGINNING OF ACTION: subtract-exit */
		{
#line 323 "parser.act"

		assert((ZIr) != NULL);

		if ((ZIexit) == NULL) {
			(ZIq) = (ZIr);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZIq) = fsm_subtract((ZIr), (ZIexit));
			if ((ZIq) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if (!fsm_trim((ZIq))) {
				perror("fsm_trim");
				goto ZL1;
			}
		}
	
#line 684 "parser.c"
		}
		/* END OF ACTION: subtract-exit */
		p_94 (lex_state, act_state);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
		/* BEGINNING OF ACTION: add-binding */
		{
#line 249 "parser.act"

		struct var *v;

		assert((ZIa) != NULL);
		assert((ZIz) != NULL);
		assert((ZIn) != NULL);
		assert((ZIq) != NULL);

		(void) (ZIa);

		v = var_bind(&(ZIz)->vl, (ZIn), (ZIq));
		if (v == NULL) {
			perror("var_bind");
			goto ZL1;
		}
	
#line 711 "parser.c"
		}
		/* END OF ACTION: add-binding */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

void
p_lx(lex_state lex_state, act_state act_state, ast *ZOa)
{
	ast ZIa;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		zone ZIz;
		fsm ZIexit;

		/* BEGINNING OF ACTION: make-ast */
		{
#line 203 "parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL1;
		}
	
#line 743 "parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: make-zone */
		{
#line 211 "parser.act"

		assert((ZIa) != NULL);

		(ZIz) = ast_addzone((ZIa));
		if ((ZIz) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 758 "parser.c"
		}
		/* END OF ACTION: make-zone */
		/* BEGINNING OF ACTION: no-exit */
		{
#line 275 "parser.act"

		(ZIexit) = NULL;
	
#line 767 "parser.c"
		}
		/* END OF ACTION: no-exit */
		/* BEGINNING OF ACTION: set-globalzone */
		{
#line 264 "parser.act"

		assert((ZIa) != NULL);
		assert((ZIz) != NULL);

		(ZIa)->global = (ZIz);
	
#line 779 "parser.c"
		}
		/* END OF ACTION: set-globalzone */
		p_list_Hof_Hthings (lex_state, act_state, ZIa, ZIz, ZIexit);
		/* BEGINNING OF INLINE: 106 */
		{
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_EOF):
					break;
				default:
					goto ZL3;
				}
				ADVANCE_LEXER;
			}
			goto ZL2;
		ZL3:;
			{
				/* BEGINNING OF ACTION: err-expected-eof */
				{
#line 396 "parser.act"

		err_expected("EOF");
	
#line 807 "parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL2:;
		}
		/* END OF INLINE: 106 */
	}
	goto ZL0;
ZL1:;
	{
		/* BEGINNING OF ACTION: make-ast */
		{
#line 203 "parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL4;
		}
	
#line 828 "parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: err-syntax */
		{
#line 364 "parser.act"

		fprintf(stderr, "syntax error\n");
		exit(EXIT_FAILURE);
	
#line 838 "parser.c"
		}
		/* END OF ACTION: err-syntax */
	}
	goto ZL0;
ZL4:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOa = ZIa;
}

static void
p_114(lex_state lex_state, act_state act_state, ast ZI108, zone ZI109, fsm ZI110, ast *ZO111, zone *ZO112, fsm *ZO113)
{
	ast ZI111;
	zone ZI112;
	fsm ZI113;

ZL2_114:;
	switch (CURRENT_TERMINAL) {
	case (TOK_IDENT): case (TOK_RESTART): case (TOK_STRSTART):
		{
			p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI108, ZI109, ZI110);
			/* BEGINNING OF INLINE: 114 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_114;
			}
			/* END OF INLINE: 114 */
		}
		/*UNREACHED*/
	default:
		{
			ZI111 = ZI108;
			ZI112 = ZI109;
			ZI113 = ZI110;
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
	*ZO111 = ZI111;
	*ZO112 = ZI112;
	*ZO113 = ZI113;
}

static void
p_118(lex_state lex_state, act_state act_state, fsm ZI116, fsm *ZO117)
{
	fsm ZI117;

ZL2_118:;
	switch (CURRENT_TERMINAL) {
	case (TOK_RESTART): case (TOK_STRSTART):
		{
			fsm ZIb;
			fsm ZIq;

			p_concaternated_Hpatterns_C_Cpattern (lex_state, act_state, &ZIb);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-concat */
			{
#line 312 "parser.act"

		assert((ZI116) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_concat((ZI116), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_concat");
			goto ZL1;
		}
	
#line 922 "parser.c"
			}
			/* END OF ACTION: op-concat */
			/* BEGINNING OF INLINE: 118 */
			ZI116 = ZIq;
			goto ZL2_118;
			/* END OF INLINE: 118 */
		}
		/*UNREACHED*/
	default:
		{
			ZI117 = ZI116;
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
	*ZO117 = ZI117;
}

static void
p_124(lex_state lex_state, act_state act_state, ast *ZIa, zone *ZIz, string *ZI122, fsm *ZI123)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_TO):
		{
			fsm ZIr2;
			string ZIt2;
			zone ZIchild;

			/* BEGINNING OF INLINE: 89 */
			{
				{
					switch (CURRENT_TERMINAL) {
					case (TOK_TO):
						break;
					default:
						goto ZL3;
					}
					ADVANCE_LEXER;
				}
				goto ZL2;
			ZL3:;
				{
					/* BEGINNING OF ACTION: err-expected-to */
					{
#line 384 "parser.act"

		err_expected("'..'");
	
#line 977 "parser.c"
					}
					/* END OF ACTION: err-expected-to */
				}
			ZL2:;
			}
			/* END OF INLINE: 89 */
			p_token_Hmapping (lex_state, act_state, &ZIr2, &ZIt2);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: make-zone */
			{
#line 211 "parser.act"

		assert((*ZIa) != NULL);

		(ZIchild) = ast_addzone((*ZIa));
		if ((ZIchild) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 1001 "parser.c"
			}
			/* END OF ACTION: make-zone */
			/* BEGINNING OF INLINE: 93 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_SEMI):
					{
						zone ZIx;
						string ZIy;
						fsm ZIu;
						fsm ZIw;
						fsm ZIv;

						p_94 (lex_state, act_state);
						if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
							RESTORE_LEXER;
							goto ZL5;
						}
						/* BEGINNING OF ACTION: no-to */
						{
#line 279 "parser.act"

		(ZIx) = NULL;
	
#line 1026 "parser.c"
						}
						/* END OF ACTION: no-to */
						/* BEGINNING OF ACTION: no-token */
						{
#line 271 "parser.act"

		(ZIy) = NULL;
	
#line 1035 "parser.c"
						}
						/* END OF ACTION: no-token */
						/* BEGINNING OF ACTION: clone */
						{
#line 348 "parser.act"

		assert((ZIr2) != NULL);

		(ZIu) = fsm_clone((ZIr2));
		if ((ZIu) == NULL) {
			perror("fsm_clone");
			goto ZL5;
		}
	
#line 1050 "parser.c"
						}
						/* END OF ACTION: clone */
						/* BEGINNING OF ACTION: regex-any */
						{
#line 182 "parser.act"

		struct fsm_state *e;

		(ZIw) = re_new_empty();
		if ((ZIw) == NULL) {
			perror("re_new_empty");
			goto ZL5;
		}

		e = fsm_addstate((ZIw));
		if (e == NULL) {
			perror("fsm_addstate");
			goto ZL5;
		}

		if (!fsm_addedge_any((ZIw), fsm_getstart((ZIw)), e)) {
			perror("fsm_addedge_any");
			goto ZL5;
		}

		fsm_setend((ZIw), e, 1);
	
#line 1078 "parser.c"
						}
						/* END OF ACTION: regex-any */
						/* BEGINNING OF ACTION: subtract-exit */
						{
#line 323 "parser.act"

		assert((ZIw) != NULL);

		if ((ZIu) == NULL) {
			(ZIv) = (ZIw);
		} else {
			(ZIu) = fsm_clone((ZIu));
			if ((ZIu) == NULL) {
				perror("fsm_clone");
				goto ZL5;
			}

			(ZIv) = fsm_subtract((ZIw), (ZIu));
			if ((ZIv) == NULL) {
				perror("fsm_subtract");
				goto ZL5;
			}

			if (!fsm_trim((ZIv))) {
				perror("fsm_trim");
				goto ZL5;
			}
		}
	
#line 1108 "parser.c"
						}
						/* END OF ACTION: subtract-exit */
						/* BEGINNING OF ACTION: add-mapping */
						{
#line 224 "parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((ZIchild) != NULL);
		assert((ZIchild) != (ZIx));
		assert((ZIv) != NULL);

		if ((ZIy) != NULL) {
			t = ast_addtoken((*ZIa), (ZIy));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL5;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((ZIchild), (ZIv), t, (ZIx));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL5;
		}
	
#line 1139 "parser.c"
						}
						/* END OF ACTION: add-mapping */
					}
					break;
				case (TOK_OPEN):
					{
						/* BEGINNING OF INLINE: 100 */
						{
							{
								switch (CURRENT_TERMINAL) {
								case (TOK_OPEN):
									break;
								default:
									goto ZL7;
								}
								ADVANCE_LEXER;
							}
							goto ZL6;
						ZL7:;
							{
								/* BEGINNING OF ACTION: err-expected-open */
								{
#line 388 "parser.act"

		err_expected("'{'");
	
#line 1166 "parser.c"
								}
								/* END OF ACTION: err-expected-open */
							}
						ZL6:;
						}
						/* END OF INLINE: 100 */
						p_list_Hof_Hthings (lex_state, act_state, *ZIa, ZIchild, ZIr2);
						/* BEGINNING OF INLINE: 101 */
						{
							if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
								RESTORE_LEXER;
								goto ZL5;
							}
							{
								switch (CURRENT_TERMINAL) {
								case (TOK_CLOSE):
									break;
								default:
									goto ZL9;
								}
								ADVANCE_LEXER;
							}
							goto ZL8;
						ZL9:;
							{
								/* BEGINNING OF ACTION: err-expected-close */
								{
#line 392 "parser.act"

		err_expected("'}'");
	
#line 1198 "parser.c"
								}
								/* END OF ACTION: err-expected-close */
							}
						ZL8:;
						}
						/* END OF INLINE: 101 */
					}
					break;
				default:
					goto ZL5;
				}
				goto ZL4;
			ZL5:;
				{
					/* BEGINNING OF ACTION: err-expected-list */
					{
#line 400 "parser.act"

		err_expected("list of token mappings or zones");
	
#line 1219 "parser.c"
					}
					/* END OF ACTION: err-expected-list */
				}
			ZL4:;
			}
			/* END OF INLINE: 93 */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 224 "parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIchild));
		assert((*ZI123) != NULL);

		if ((*ZI122) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI122));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI123), t, (ZIchild));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 1254 "parser.c"
			}
			/* END OF ACTION: add-mapping */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 224 "parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((ZIchild) != NULL);
		assert((ZIchild) != (*ZIz));
		assert((ZIr2) != NULL);

		if ((ZIt2) != NULL) {
			t = ast_addtoken((*ZIa), (ZIt2));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((ZIchild), (ZIr2), t, (*ZIz));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 1285 "parser.c"
			}
			/* END OF ACTION: add-mapping */
		}
		break;
	case (TOK_SEMI):
		{
			zone ZIto;

			p_94 (lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: no-to */
			{
#line 279 "parser.act"

		(ZIto) = NULL;
	
#line 1305 "parser.c"
			}
			/* END OF ACTION: no-to */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 224 "parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIto));
		assert((*ZI123) != NULL);

		if ((*ZI122) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI122));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI123), t, (ZIto));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 1336 "parser.c"
			}
			/* END OF ACTION: add-mapping */
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

#line 445 "parser.act"


	struct ast *lx_parse(FILE *f) {
		struct act_state act_state_s;
		struct act_state *act_state;
		struct lex_state lex_state_s;
		struct lex_state *lex_state;
		struct ast *ast;
		struct lx *lx;

		assert(f != NULL);

		lex_state = &lex_state_s;

		lex_state->p = lex_state->a;

		lx = &lex_state->lx;

		lx_init(lx);

		lx->lgetc  = lx_fgetc;
		lx->opaque = f;

		lex_state->buf.a   = NULL;
		lex_state->buf.len = 0;

		lx->buf   = &lex_state->buf;
		lx->push  = lx_dynpush;
		lx->pop   = lx_dynpop;
		lx->clear = lx_dynclear;
		lx->free  = lx_dynfree;

		/* This is a workaround for ADVANCE_LEXER assuming a pointer */
		act_state = &act_state_s;

		ADVANCE_LEXER;	/* XXX: what if the first token is unrecognised? */
		p_lx(lex_state, act_state, &ast);

		assert(ast != NULL);

		return ast;
	}

#line 1398 "parser.c"

/* END OF FILE */
