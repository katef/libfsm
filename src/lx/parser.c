/*
 * Automatically generated from the files:
 *	parser.sid
 * and
 *	parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 74 "parser.act"


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

#line 67 "parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Hthings(lex_state, act_state, ast, zone, fsm);
static void p_token_Hmapping_C_Cpattern__literal(lex_state, act_state);
static void p_list_Hof_Hthings_C_Cthing(lex_state, act_state, ast, zone, fsm);
static void p_token_Hmapping_C_Cpattern__regex(lex_state, act_state);
static void p_token_Hmapping_C_Cpattern(lex_state, act_state, fsm *);
static void p_token_Hmapping(lex_state, act_state, fsm *, string *);
static void p_token_Hmapping_C_Cconcaternated_Hpatterns(lex_state, act_state, fsm *);
static void p_86(lex_state, act_state);
extern void p_lx(lex_state, act_state, ast *);
static void p_106(lex_state, act_state, ast, zone, fsm, ast *, zone *, fsm *);
static void p_110(lex_state, act_state, fsm, fsm *);
static void p_116(lex_state, act_state, ast *, zone *, string *, fsm *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Hthings(lex_state lex_state, act_state act_state, ast ZI100, zone ZI101, fsm ZI102)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		ast ZI103;
		zone ZI104;
		fsm ZI105;

		p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI100, ZI101, ZI102);
		p_106 (lex_state, act_state, ZI100, ZI101, ZI102, &ZI103, &ZI104, &ZI105);
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
#line 369 "parser.act"

		err_expected("list of token mappings or zones");
	
#line 121 "parser.c"
		}
		/* END OF ACTION: err-expected-list */
	}
}

static void
p_token_Hmapping_C_Cpattern__literal(lex_state lex_state, act_state act_state)
{
ZL2_token_Hmapping_C_Cpattern__literal:;
	switch (CURRENT_TERMINAL) {
	case (TOK_CHAR):
		{
			char ZIc;

			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 108 "parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIc = lex_state->buf.a[0];
	
#line 145 "parser.c"
			}
			/* END OF EXTRACT: CHAR */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: pattern-char */
			{
#line 123 "parser.act"

		*lex_state->p++ = (ZIc);
	
#line 155 "parser.c"
			}
			/* END OF ACTION: pattern-char */
			/* BEGINNING OF INLINE: token-mapping::pattern_literal */
			goto ZL2_token_Hmapping_C_Cpattern__literal;
			/* END OF INLINE: token-mapping::pattern_literal */
		}
		/*UNREACHED*/
	case (TOK_ESC):
		{
			char ZIc;

			/* BEGINNING OF EXTRACT: ESC */
			{
#line 96 "parser.act"

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
	
#line 183 "parser.c"
			}
			/* END OF EXTRACT: ESC */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: pattern-char */
			{
#line 123 "parser.act"

		*lex_state->p++ = (ZIc);
	
#line 193 "parser.c"
			}
			/* END OF ACTION: pattern-char */
			/* BEGINNING OF INLINE: token-mapping::pattern_literal */
			goto ZL2_token_Hmapping_C_Cpattern__literal;
			/* END OF INLINE: token-mapping::pattern_literal */
		}
		/*UNREACHED*/
	case (ERROR_TERMINAL):
		return;
	default:
		break;
	}
}

static void
p_list_Hof_Hthings_C_Cthing(lex_state lex_state, act_state act_state, ast ZIa, zone ZIz, fsm ZIexit)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZI113;
		string ZI114;
		fsm ZI115;

		p_token_Hmapping (lex_state, act_state, &ZI113, &ZI114);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
		/* BEGINNING OF ACTION: subtract-exit */
		{
#line 296 "parser.act"

		assert((ZI113) != NULL);

		if ((ZIexit) == NULL) {
			(ZI115) = (ZI113);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI115) = fsm_subtract((ZI113), (ZIexit));
			if ((ZI115) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if (!fsm_trim((ZI115))) {
				perror("fsm_trim");
				goto ZL1;
			}
		}
	
#line 251 "parser.c"
		}
		/* END OF ACTION: subtract-exit */
		p_116 (lex_state, act_state, &ZIa, &ZIz, &ZI114, &ZI115);
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
p_token_Hmapping_C_Cpattern__regex(lex_state lex_state, act_state act_state)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_CHAR):
		{
			char ZIc;

			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 108 "parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIc = lex_state->buf.a[0];
	
#line 283 "parser.c"
			}
			/* END OF EXTRACT: CHAR */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: pattern-char */
			{
#line 123 "parser.act"

		*lex_state->p++ = (ZIc);
	
#line 293 "parser.c"
			}
			/* END OF ACTION: pattern-char */
			p_token_Hmapping_C_Cpattern__literal (lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (ERROR_TERMINAL):
		return;
	default:
		break;
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_token_Hmapping_C_Cpattern(lex_state lex_state, act_state act_state, fsm *ZOr)
{
	fsm ZIr;

	switch (CURRENT_TERMINAL) {
	case (TOK_RESTART):
		{
			string ZIs;

			ADVANCE_LEXER;
			p_token_Hmapping_C_Cpattern__regex (lex_state, act_state);
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
#line 130 "parser.act"

		/* TODO */
		*lex_state->p++ = '\0';

		(ZIs) = lex_state->a;

		lex_state->p = lex_state->a;
	
#line 347 "parser.c"
			}
			/* END OF ACTION: pattern-buffer */
			/* BEGINNING OF ACTION: compile-regex */
			{
#line 147 "parser.act"

		assert((ZIs) != NULL);

		/* TODO: handle error codes */
		(ZIr) = re_new_comp(RE_SIMPLE, re_getc_str, &(ZIs), 0, NULL);
		if ((ZIr) == NULL) {
			/* TODO: use *err */
			goto ZL1;
		}
	
#line 363 "parser.c"
			}
			/* END OF ACTION: compile-regex */
		}
		break;
	case (TOK_STRSTART):
		{
			string ZIs;

			ADVANCE_LEXER;
			p_token_Hmapping_C_Cpattern__literal (lex_state, act_state);
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
#line 130 "parser.act"

		/* TODO */
		*lex_state->p++ = '\0';

		(ZIs) = lex_state->a;

		lex_state->p = lex_state->a;
	
#line 395 "parser.c"
			}
			/* END OF ACTION: pattern-buffer */
			/* BEGINNING OF ACTION: compile-literal */
			{
#line 136 "parser.act"

		assert((ZIs) != NULL);

		/* TODO: handle error codes */
		(ZIr) = re_new_comp(RE_LITERAL, re_getc_str, &(ZIs), 0, NULL);
		if ((ZIr) == NULL) {
			/* TODO: use *err */
			goto ZL1;
		}
	
#line 411 "parser.c"
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
p_token_Hmapping(lex_state lex_state, act_state act_state, fsm *ZOr, string *ZOt)
{
	fsm ZIr;
	string ZIt;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_token_Hmapping_C_Cconcaternated_Hpatterns (lex_state, act_state, &ZIr);
		/* BEGINNING OF INLINE: 67 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_MAP):
				{
					/* BEGINNING OF INLINE: 68 */
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
#line 345 "parser.act"

		err_expected("'->'");
	
#line 465 "parser.c"
							}
							/* END OF ACTION: err-expected-map */
						}
					ZL3:;
					}
					/* END OF INLINE: 68 */
					switch (CURRENT_TERMINAL) {
					case (TOK_TOKEN):
						/* BEGINNING OF EXTRACT: TOKEN */
						{
#line 113 "parser.act"

		/* TODO: submatch addressing */
		ZIt = xstrdup(lex_state->buf.a + 1); /* +1 for '$' prefix */
		if (ZIt == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 485 "parser.c"
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
#line 244 "parser.act"

		(ZIt) = NULL;
	
#line 503 "parser.c"
					}
					/* END OF ACTION: no-token */
				}
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 67 */
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
p_token_Hmapping_C_Cconcaternated_Hpatterns(lex_state lex_state, act_state act_state, fsm *ZO109)
{
	fsm ZI109;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIq;

		p_token_Hmapping_C_Cpattern (lex_state, act_state, &ZIq);
		p_110 (lex_state, act_state, ZIq, &ZI109);
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
	*ZO109 = ZI109;
}

static void
p_86(lex_state lex_state, act_state act_state)
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
#line 349 "parser.act"

		err_expected("';'");
	
#line 574 "parser.c"
		}
		/* END OF ACTION: err-expected-semi */
	}
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
#line 193 "parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL1;
		}
	
#line 602 "parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: make-zone */
		{
#line 201 "parser.act"

		assert((ZIa) != NULL);

		(ZIz) = ast_addzone((ZIa));
		if ((ZIz) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 617 "parser.c"
		}
		/* END OF ACTION: make-zone */
		/* BEGINNING OF ACTION: no-exit */
		{
#line 248 "parser.act"

		(ZIexit) = NULL;
	
#line 626 "parser.c"
		}
		/* END OF ACTION: no-exit */
		/* BEGINNING OF ACTION: set-globalzone */
		{
#line 237 "parser.act"

		assert((ZIa) != NULL);
		assert((ZIz) != NULL);

		(ZIa)->global = (ZIz);
	
#line 638 "parser.c"
		}
		/* END OF ACTION: set-globalzone */
		p_list_Hof_Hthings (lex_state, act_state, ZIa, ZIz, ZIexit);
		/* BEGINNING OF INLINE: 98 */
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
#line 365 "parser.act"

		err_expected("EOF");
	
#line 666 "parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL2:;
		}
		/* END OF INLINE: 98 */
	}
	goto ZL0;
ZL1:;
	{
		/* BEGINNING OF ACTION: make-ast */
		{
#line 193 "parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL4;
		}
	
#line 687 "parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: err-syntax */
		{
#line 337 "parser.act"

		fprintf(stderr, "syntax error\n");
		exit(EXIT_FAILURE);
	
#line 697 "parser.c"
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
p_106(lex_state lex_state, act_state act_state, ast ZI100, zone ZI101, fsm ZI102, ast *ZO103, zone *ZO104, fsm *ZO105)
{
	ast ZI103;
	zone ZI104;
	fsm ZI105;

ZL2_106:;
	switch (CURRENT_TERMINAL) {
	case (TOK_RESTART): case (TOK_STRSTART):
		{
			p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI100, ZI101, ZI102);
			/* BEGINNING OF INLINE: 106 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_106;
			}
			/* END OF INLINE: 106 */
		}
		/*UNREACHED*/
	default:
		{
			ZI103 = ZI100;
			ZI104 = ZI101;
			ZI105 = ZI102;
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
	*ZO104 = ZI104;
	*ZO105 = ZI105;
}

static void
p_110(lex_state lex_state, act_state act_state, fsm ZI108, fsm *ZO109)
{
	fsm ZI109;

ZL2_110:;
	switch (CURRENT_TERMINAL) {
	case (TOK_RESTART): case (TOK_STRSTART):
		{
			fsm ZIb;
			fsm ZIq;

			p_token_Hmapping_C_Cpattern (lex_state, act_state, &ZIb);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-concat */
			{
#line 285 "parser.act"

		assert((ZI108) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_concat((ZI108), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_concat");
			goto ZL1;
		}
	
#line 781 "parser.c"
			}
			/* END OF ACTION: op-concat */
			/* BEGINNING OF INLINE: 110 */
			ZI108 = ZIq;
			goto ZL2_110;
			/* END OF INLINE: 110 */
		}
		/*UNREACHED*/
	default:
		{
			ZI109 = ZI108;
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
	*ZO109 = ZI109;
}

static void
p_116(lex_state lex_state, act_state act_state, ast *ZIa, zone *ZIz, string *ZI114, fsm *ZI115)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_TO):
		{
			fsm ZIr2;
			string ZIt2;
			zone ZIchild;

			/* BEGINNING OF INLINE: 81 */
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
#line 353 "parser.act"

		err_expected("'..'");
	
#line 836 "parser.c"
					}
					/* END OF ACTION: err-expected-to */
				}
			ZL2:;
			}
			/* END OF INLINE: 81 */
			p_token_Hmapping (lex_state, act_state, &ZIr2, &ZIt2);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: make-zone */
			{
#line 201 "parser.act"

		assert((*ZIa) != NULL);

		(ZIchild) = ast_addzone((*ZIa));
		if ((ZIchild) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 860 "parser.c"
			}
			/* END OF ACTION: make-zone */
			/* BEGINNING OF INLINE: 85 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_SEMI):
					{
						zone ZIx;
						string ZIy;
						fsm ZIu;
						fsm ZIw;
						fsm ZIv;

						p_86 (lex_state, act_state);
						if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
							RESTORE_LEXER;
							goto ZL5;
						}
						/* BEGINNING OF ACTION: no-to */
						{
#line 252 "parser.act"

		(ZIx) = NULL;
	
#line 885 "parser.c"
						}
						/* END OF ACTION: no-to */
						/* BEGINNING OF ACTION: no-token */
						{
#line 244 "parser.act"

		(ZIy) = NULL;
	
#line 894 "parser.c"
						}
						/* END OF ACTION: no-token */
						/* BEGINNING OF ACTION: clone */
						{
#line 321 "parser.act"

		assert((ZIr2) != NULL);

		(ZIu) = fsm_clone((ZIr2));
		if ((ZIu) == NULL) {
			perror("fsm_clone");
			goto ZL5;
		}
	
#line 909 "parser.c"
						}
						/* END OF ACTION: clone */
						/* BEGINNING OF ACTION: regex-any */
						{
#line 172 "parser.act"

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
	
#line 937 "parser.c"
						}
						/* END OF ACTION: regex-any */
						/* BEGINNING OF ACTION: subtract-exit */
						{
#line 296 "parser.act"

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
	
#line 967 "parser.c"
						}
						/* END OF ACTION: subtract-exit */
						/* BEGINNING OF ACTION: make-mapping */
						{
#line 214 "parser.act"

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
	
#line 998 "parser.c"
						}
						/* END OF ACTION: make-mapping */
					}
					break;
				case (TOK_OPEN):
					{
						/* BEGINNING OF INLINE: 92 */
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
#line 357 "parser.act"

		err_expected("'{'");
	
#line 1025 "parser.c"
								}
								/* END OF ACTION: err-expected-open */
							}
						ZL6:;
						}
						/* END OF INLINE: 92 */
						p_list_Hof_Hthings (lex_state, act_state, *ZIa, ZIchild, ZIr2);
						/* BEGINNING OF INLINE: 93 */
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
#line 361 "parser.act"

		err_expected("'}'");
	
#line 1057 "parser.c"
								}
								/* END OF ACTION: err-expected-close */
							}
						ZL8:;
						}
						/* END OF INLINE: 93 */
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
#line 369 "parser.act"

		err_expected("list of token mappings or zones");
	
#line 1078 "parser.c"
					}
					/* END OF ACTION: err-expected-list */
				}
			ZL4:;
			}
			/* END OF INLINE: 85 */
			/* BEGINNING OF ACTION: make-mapping */
			{
#line 214 "parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIchild));
		assert((*ZI115) != NULL);

		if ((*ZI114) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI114));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI115), t, (ZIchild));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 1113 "parser.c"
			}
			/* END OF ACTION: make-mapping */
			/* BEGINNING OF ACTION: make-mapping */
			{
#line 214 "parser.act"

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
	
#line 1144 "parser.c"
			}
			/* END OF ACTION: make-mapping */
		}
		break;
	case (TOK_SEMI):
		{
			zone ZIto;

			p_86 (lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: no-to */
			{
#line 252 "parser.act"

		(ZIto) = NULL;
	
#line 1164 "parser.c"
			}
			/* END OF ACTION: no-to */
			/* BEGINNING OF ACTION: make-mapping */
			{
#line 214 "parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIto));
		assert((*ZI115) != NULL);

		if ((*ZI114) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI114));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI115), t, (ZIto));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 1195 "parser.c"
			}
			/* END OF ACTION: make-mapping */
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

#line 414 "parser.act"


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

#line 1257 "parser.c"

/* END OF FILE */
