/*
 * Automatically generated from the files:
 *	parser.sid
 * and
 *	parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 77 "parser.act"


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
	typedef enum re_cflags       flags;
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

#line 69 "parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Hthings(lex_state, act_state, ast, zone, fsm);
static void p_concaternated_Hpatterns_C_Cpattern(lex_state, act_state, zone, fsm *);
static void p_concaternated_Hpatterns(lex_state, act_state, zone, fsm *);
static void p_concaternated_Hpatterns_C_Cpattern_Hescaped(lex_state, act_state);
static void p_141(lex_state, act_state, ast *, zone *, string *, fsm *);
static void p_concaternated_Hpatterns_C_Cpattern_Hverbatim(lex_state, act_state);
static void p_list_Hof_Hthings_C_Cthing(lex_state, act_state, ast, zone, fsm);
static void p_token_Hmapping(lex_state, act_state, zone, fsm *, string *);
static void p_75(lex_state, act_state, string *);
static void p_97(lex_state, act_state);
extern void p_lx(lex_state, act_state, ast *);
static void p_117(lex_state, act_state, ast, zone, fsm, ast *, zone *, fsm *);
static void p_123(lex_state, act_state, zone, fsm, zone *, fsm *);
static void p_126(lex_state, act_state, ast *, zone *, fsm *, string *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Hthings(lex_state lex_state, act_state act_state, ast ZI111, zone ZI112, fsm ZI113)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		ast ZI114;
		zone ZI115;
		fsm ZI116;

		p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI111, ZI112, ZI113);
		p_117 (lex_state, act_state, ZI111, ZI112, ZI113, &ZI114, &ZI115, &ZI116);
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
#line 432 "parser.act"

		err_expected("list of token mappings or zones");
	
#line 125 "parser.c"
		}
		/* END OF ACTION: err-expected-list */
	}
}

static void
p_concaternated_Hpatterns_C_Cpattern(lex_state lex_state, act_state act_state, zone ZIz, fsm *ZOr)
{
	fsm ZIr;

	switch (CURRENT_TERMINAL) {
	case (TOK_IDENT):
		{
			string ZIn;

			/* BEGINNING OF EXTRACT: IDENT */
			{
#line 124 "parser.act"

		ZIn = xstrdup(lex_state->buf.a);
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 151 "parser.c"
			}
			/* END OF EXTRACT: IDENT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: deref-var */
			{
#line 158 "parser.act"

		assert((ZIn) != NULL);

		/* TODO: walk zone tree to root */

		(ZIr) = var_find((ZIz)->vl, (ZIn));
		if ((ZIr) == NULL) {
			/* TODO: use *err */
			fprintf(stderr, "No such variable: %s\n", (ZIn));
			goto ZL1;
		}

		(ZIr) = fsm_clone((ZIr));
		if ((ZIr) == NULL) {
			/* TODO: use *err */
			perror("fsm_clone");
			goto ZL1;
		}
	
#line 177 "parser.c"
			}
			/* END OF ACTION: deref-var */
		}
		break;
	case (TOK_RESTART):
		{
			flags ZIf;
			string ZIs;

			ADVANCE_LEXER;
			p_concaternated_Hpatterns_C_Cpattern_Hverbatim (lex_state, act_state);
			switch (CURRENT_TERMINAL) {
			case (TOK_REEND):
				/* BEGINNING OF EXTRACT: REEND */
				{
#line 135 "parser.act"

		assert(lex_state->buf.a[0] == '/');

		/* TODO: submatch addressing */
		if (-1 == re_cflags(lex_state->buf.a + 1, &ZIf)) { /* TODO: +1 for '/' prefix */
			fprintf(stderr, "Unrecognised regexp flags: %s\n", lex_state->buf.a + 1);
			/* TODO: raise parse error */
		}
	
#line 203 "parser.c"
				}
				/* END OF EXTRACT: REEND */
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
#line 152 "parser.act"

		/* TODO */
		*lex_state->p++ = '\0';

		(ZIs) = lex_state->a;

		lex_state->p = lex_state->a;
	
#line 225 "parser.c"
			}
			/* END OF ACTION: pattern-buffer */
			/* BEGINNING OF ACTION: compile-regex */
			{
#line 189 "parser.act"

		assert((ZIs) != NULL);

		/* TODO: handle error codes */
		(ZIr) = re_new_comp(RE_SIMPLE, re_getc_str, &(ZIs), (ZIf), NULL);
		if ((ZIr) == NULL) {
			/* TODO: use *err */
			goto ZL1;
		}
	
#line 241 "parser.c"
			}
			/* END OF ACTION: compile-regex */
		}
		break;
	case (TOK_STRSTART):
		{
			string ZIs;

			ADVANCE_LEXER;
			p_concaternated_Hpatterns_C_Cpattern_Hescaped (lex_state, act_state);
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
#line 152 "parser.act"

		/* TODO */
		*lex_state->p++ = '\0';

		(ZIs) = lex_state->a;

		lex_state->p = lex_state->a;
	
#line 273 "parser.c"
			}
			/* END OF ACTION: pattern-buffer */
			/* BEGINNING OF ACTION: compile-literal */
			{
#line 178 "parser.act"

		assert((ZIs) != NULL);

		/* TODO: handle error codes */
		(ZIr) = re_new_comp(RE_LITERAL, re_getc_str, &(ZIs), 0, NULL);
		if ((ZIr) == NULL) {
			/* TODO: use *err */
			goto ZL1;
		}
	
#line 289 "parser.c"
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
p_concaternated_Hpatterns(lex_state lex_state, act_state act_state, zone ZI119, fsm *ZO122)
{
	fsm ZI122;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		fsm ZIq;
		zone ZI121;

		p_concaternated_Hpatterns_C_Cpattern (lex_state, act_state, ZI119, &ZIq);
		p_123 (lex_state, act_state, ZI119, ZIq, &ZI121, &ZI122);
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
	*ZO122 = ZI122;
}

static void
p_concaternated_Hpatterns_C_Cpattern_Hescaped(lex_state lex_state, act_state act_state)
{
ZL2_concaternated_Hpatterns_C_Cpattern_Hescaped:;
	switch (CURRENT_TERMINAL) {
	case (TOK_CHAR):
		{
			char ZIc;

			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 111 "parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIc = lex_state->buf.a[0];
	
#line 352 "parser.c"
			}
			/* END OF EXTRACT: CHAR */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: pattern-char */
			{
#line 145 "parser.act"

		/* TODO */
		*lex_state->p++ = (ZIc);
	
#line 363 "parser.c"
			}
			/* END OF ACTION: pattern-char */
			/* BEGINNING OF INLINE: concaternated-patterns::pattern-escaped */
			goto ZL2_concaternated_Hpatterns_C_Cpattern_Hescaped;
			/* END OF INLINE: concaternated-patterns::pattern-escaped */
		}
		/*UNREACHED*/
	case (TOK_ESC):
		{
			char ZIc;

			/* BEGINNING OF EXTRACT: ESC */
			{
#line 99 "parser.act"

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
	
#line 391 "parser.c"
			}
			/* END OF EXTRACT: ESC */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: pattern-char */
			{
#line 145 "parser.act"

		/* TODO */
		*lex_state->p++ = (ZIc);
	
#line 402 "parser.c"
			}
			/* END OF ACTION: pattern-char */
			/* BEGINNING OF INLINE: concaternated-patterns::pattern-escaped */
			goto ZL2_concaternated_Hpatterns_C_Cpattern_Hescaped;
			/* END OF INLINE: concaternated-patterns::pattern-escaped */
		}
		/*UNREACHED*/
	case (ERROR_TERMINAL):
		return;
	default:
		break;
	}
}

static void
p_141(lex_state lex_state, act_state act_state, ast *ZIa, zone *ZIz, string *ZI139, fsm *ZI140)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_TO):
		{
			fsm ZIr2;
			string ZIt2;
			zone ZIchild;

			/* BEGINNING OF INLINE: 92 */
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
#line 416 "parser.act"

		err_expected("'..'");
	
#line 447 "parser.c"
					}
					/* END OF ACTION: err-expected-to */
				}
			ZL2:;
			}
			/* END OF INLINE: 92 */
			p_token_Hmapping (lex_state, act_state, *ZIz, &ZIr2, &ZIt2);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: make-zone */
			{
#line 243 "parser.act"

		assert((*ZIa) != NULL);

		(ZIchild) = ast_addzone((*ZIa));
		if ((ZIchild) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 471 "parser.c"
			}
			/* END OF ACTION: make-zone */
			/* BEGINNING OF INLINE: 96 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_SEMI):
					{
						zone ZIx;
						string ZIy;
						fsm ZIu;
						fsm ZIw;
						fsm ZIv;

						p_97 (lex_state, act_state);
						if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
							RESTORE_LEXER;
							goto ZL5;
						}
						/* BEGINNING OF ACTION: no-to */
						{
#line 311 "parser.act"

		(ZIx) = NULL;
	
#line 496 "parser.c"
						}
						/* END OF ACTION: no-to */
						/* BEGINNING OF ACTION: no-token */
						{
#line 303 "parser.act"

		(ZIy) = NULL;
	
#line 505 "parser.c"
						}
						/* END OF ACTION: no-token */
						/* BEGINNING OF ACTION: clone */
						{
#line 380 "parser.act"

		assert((ZIr2) != NULL);

		(ZIu) = fsm_clone((ZIr2));
		if ((ZIu) == NULL) {
			perror("fsm_clone");
			goto ZL5;
		}
	
#line 520 "parser.c"
						}
						/* END OF ACTION: clone */
						/* BEGINNING OF ACTION: regex-any */
						{
#line 214 "parser.act"

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
	
#line 548 "parser.c"
						}
						/* END OF ACTION: regex-any */
						/* BEGINNING OF ACTION: subtract-exit */
						{
#line 355 "parser.act"

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
	
#line 578 "parser.c"
						}
						/* END OF ACTION: subtract-exit */
						/* BEGINNING OF ACTION: add-mapping */
						{
#line 256 "parser.act"

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
	
#line 609 "parser.c"
						}
						/* END OF ACTION: add-mapping */
					}
					break;
				case (TOK_OPEN):
					{
						/* BEGINNING OF INLINE: 103 */
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
#line 420 "parser.act"

		err_expected("'{'");
	
#line 636 "parser.c"
								}
								/* END OF ACTION: err-expected-open */
							}
						ZL6:;
						}
						/* END OF INLINE: 103 */
						p_list_Hof_Hthings (lex_state, act_state, *ZIa, ZIchild, ZIr2);
						/* BEGINNING OF INLINE: 104 */
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
#line 424 "parser.act"

		err_expected("'}'");
	
#line 668 "parser.c"
								}
								/* END OF ACTION: err-expected-close */
							}
						ZL8:;
						}
						/* END OF INLINE: 104 */
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
#line 432 "parser.act"

		err_expected("list of token mappings or zones");
	
#line 689 "parser.c"
					}
					/* END OF ACTION: err-expected-list */
				}
			ZL4:;
			}
			/* END OF INLINE: 96 */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 256 "parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIchild));
		assert((*ZI140) != NULL);

		if ((*ZI139) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI139));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI140), t, (ZIchild));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 724 "parser.c"
			}
			/* END OF ACTION: add-mapping */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 256 "parser.act"

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
	
#line 755 "parser.c"
			}
			/* END OF ACTION: add-mapping */
		}
		break;
	case (TOK_SEMI):
		{
			zone ZIto;

			p_97 (lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: no-to */
			{
#line 311 "parser.act"

		(ZIto) = NULL;
	
#line 775 "parser.c"
			}
			/* END OF ACTION: no-to */
			/* BEGINNING OF ACTION: add-mapping */
			{
#line 256 "parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIto));
		assert((*ZI140) != NULL);

		if ((*ZI139) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI139));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI140), t, (ZIto));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 806 "parser.c"
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

static void
p_concaternated_Hpatterns_C_Cpattern_Hverbatim(lex_state lex_state, act_state act_state)
{
ZL2_concaternated_Hpatterns_C_Cpattern_Hverbatim:;
	switch (CURRENT_TERMINAL) {
	case (TOK_CHAR):
		{
			char ZIc;

			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 111 "parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIc = lex_state->buf.a[0];
	
#line 840 "parser.c"
			}
			/* END OF EXTRACT: CHAR */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: pattern-char */
			{
#line 145 "parser.act"

		/* TODO */
		*lex_state->p++ = (ZIc);
	
#line 851 "parser.c"
			}
			/* END OF ACTION: pattern-char */
			/* BEGINNING OF INLINE: concaternated-patterns::pattern-verbatim */
			goto ZL2_concaternated_Hpatterns_C_Cpattern_Hverbatim;
			/* END OF INLINE: concaternated-patterns::pattern-verbatim */
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
	switch (CURRENT_TERMINAL) {
	case (TOK_IDENT):
		{
			string ZIn;

			/* BEGINNING OF EXTRACT: IDENT */
			{
#line 124 "parser.act"

		ZIn = xstrdup(lex_state->buf.a);
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 884 "parser.c"
			}
			/* END OF EXTRACT: IDENT */
			ADVANCE_LEXER;
			p_126 (lex_state, act_state, &ZIa, &ZIz, &ZIexit, &ZIn);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_RESTART):
		{
			flags ZIf;
			string ZIs;
			fsm ZIq;
			zone ZI132;
			fsm ZI133;
			string ZI134;
			fsm ZI135;

			ADVANCE_LEXER;
			p_concaternated_Hpatterns_C_Cpattern_Hverbatim (lex_state, act_state);
			switch (CURRENT_TERMINAL) {
			case (TOK_REEND):
				/* BEGINNING OF EXTRACT: REEND */
				{
#line 135 "parser.act"

		assert(lex_state->buf.a[0] == '/');

		/* TODO: submatch addressing */
		if (-1 == re_cflags(lex_state->buf.a + 1, &ZIf)) { /* TODO: +1 for '/' prefix */
			fprintf(stderr, "Unrecognised regexp flags: %s\n", lex_state->buf.a + 1);
			/* TODO: raise parse error */
		}
	
#line 921 "parser.c"
				}
				/* END OF EXTRACT: REEND */
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
#line 152 "parser.act"

		/* TODO */
		*lex_state->p++ = '\0';

		(ZIs) = lex_state->a;

		lex_state->p = lex_state->a;
	
#line 943 "parser.c"
			}
			/* END OF ACTION: pattern-buffer */
			/* BEGINNING OF ACTION: compile-regex */
			{
#line 189 "parser.act"

		assert((ZIs) != NULL);

		/* TODO: handle error codes */
		(ZIq) = re_new_comp(RE_SIMPLE, re_getc_str, &(ZIs), (ZIf), NULL);
		if ((ZIq) == NULL) {
			/* TODO: use *err */
			goto ZL1;
		}
	
#line 959 "parser.c"
			}
			/* END OF ACTION: compile-regex */
			p_123 (lex_state, act_state, ZIz, ZIq, &ZI132, &ZI133);
			p_75 (lex_state, act_state, &ZI134);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 355 "parser.act"

		assert((ZI133) != NULL);

		if ((ZIexit) == NULL) {
			(ZI135) = (ZI133);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI135) = fsm_subtract((ZI133), (ZIexit));
			if ((ZI135) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if (!fsm_trim((ZI135))) {
				perror("fsm_trim");
				goto ZL1;
			}
		}
	
#line 995 "parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_141 (lex_state, act_state, &ZIa, &ZIz, &ZI134, &ZI135);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_STRSTART):
		{
			string ZIs;
			fsm ZIq;
			zone ZI127;
			fsm ZI128;
			string ZI129;
			fsm ZI130;

			ADVANCE_LEXER;
			p_concaternated_Hpatterns_C_Cpattern_Hescaped (lex_state, act_state);
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
#line 152 "parser.act"

		/* TODO */
		*lex_state->p++ = '\0';

		(ZIs) = lex_state->a;

		lex_state->p = lex_state->a;
	
#line 1037 "parser.c"
			}
			/* END OF ACTION: pattern-buffer */
			/* BEGINNING OF ACTION: compile-literal */
			{
#line 178 "parser.act"

		assert((ZIs) != NULL);

		/* TODO: handle error codes */
		(ZIq) = re_new_comp(RE_LITERAL, re_getc_str, &(ZIs), 0, NULL);
		if ((ZIq) == NULL) {
			/* TODO: use *err */
			goto ZL1;
		}
	
#line 1053 "parser.c"
			}
			/* END OF ACTION: compile-literal */
			p_123 (lex_state, act_state, ZIz, ZIq, &ZI127, &ZI128);
			p_75 (lex_state, act_state, &ZI129);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 355 "parser.act"

		assert((ZI128) != NULL);

		if ((ZIexit) == NULL) {
			(ZI130) = (ZI128);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI130) = fsm_subtract((ZI128), (ZIexit));
			if ((ZI130) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if (!fsm_trim((ZI130))) {
				perror("fsm_trim");
				goto ZL1;
			}
		}
	
#line 1089 "parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_141 (lex_state, act_state, &ZIa, &ZIz, &ZI129, &ZI130);
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
p_token_Hmapping(lex_state lex_state, act_state act_state, zone ZIz, fsm *ZOr, string *ZOt)
{
	fsm ZIr;
	string ZIt;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_concaternated_Hpatterns (lex_state, act_state, ZIz, &ZIr);
		p_75 (lex_state, act_state, &ZIt);
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
	*ZOr = ZIr;
	*ZOt = ZIt;
}

static void
p_75(lex_state lex_state, act_state act_state, string *ZOt)
{
	string ZIt;

	switch (CURRENT_TERMINAL) {
	case (TOK_MAP):
		{
			/* BEGINNING OF INLINE: 76 */
			{
				{
					switch (CURRENT_TERMINAL) {
					case (TOK_MAP):
						break;
					default:
						goto ZL3;
					}
					ADVANCE_LEXER;
				}
				goto ZL2;
			ZL3:;
				{
					/* BEGINNING OF ACTION: err-expected-map */
					{
#line 404 "parser.act"

		err_expected("'->'");
	
#line 1164 "parser.c"
					}
					/* END OF ACTION: err-expected-map */
				}
			ZL2:;
			}
			/* END OF INLINE: 76 */
			switch (CURRENT_TERMINAL) {
			case (TOK_TOKEN):
				/* BEGINNING OF EXTRACT: TOKEN */
				{
#line 116 "parser.act"

		/* TODO: submatch addressing */
		ZIt = xstrdup(lex_state->buf.a + 1); /* +1 for '$' prefix */
		if (ZIt == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 1184 "parser.c"
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
#line 303 "parser.act"

		(ZIt) = NULL;
	
#line 1202 "parser.c"
			}
			/* END OF ACTION: no-token */
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
	*ZOt = ZIt;
}

static void
p_97(lex_state lex_state, act_state act_state)
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
#line 412 "parser.act"

		err_expected("';'");
	
#line 1242 "parser.c"
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
#line 235 "parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL1;
		}
	
#line 1270 "parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: make-zone */
		{
#line 243 "parser.act"

		assert((ZIa) != NULL);

		(ZIz) = ast_addzone((ZIa));
		if ((ZIz) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 1285 "parser.c"
		}
		/* END OF ACTION: make-zone */
		/* BEGINNING OF ACTION: no-exit */
		{
#line 307 "parser.act"

		(ZIexit) = NULL;
	
#line 1294 "parser.c"
		}
		/* END OF ACTION: no-exit */
		/* BEGINNING OF ACTION: set-globalzone */
		{
#line 296 "parser.act"

		assert((ZIa) != NULL);
		assert((ZIz) != NULL);

		(ZIa)->global = (ZIz);
	
#line 1306 "parser.c"
		}
		/* END OF ACTION: set-globalzone */
		p_list_Hof_Hthings (lex_state, act_state, ZIa, ZIz, ZIexit);
		/* BEGINNING OF INLINE: 109 */
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
#line 428 "parser.act"

		err_expected("EOF");
	
#line 1334 "parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL2:;
		}
		/* END OF INLINE: 109 */
	}
	goto ZL0;
ZL1:;
	{
		/* BEGINNING OF ACTION: make-ast */
		{
#line 235 "parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL4;
		}
	
#line 1355 "parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: err-syntax */
		{
#line 396 "parser.act"

		fprintf(stderr, "syntax error\n");
		exit(EXIT_FAILURE);
	
#line 1365 "parser.c"
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
p_117(lex_state lex_state, act_state act_state, ast ZI111, zone ZI112, fsm ZI113, ast *ZO114, zone *ZO115, fsm *ZO116)
{
	ast ZI114;
	zone ZI115;
	fsm ZI116;

ZL2_117:;
	switch (CURRENT_TERMINAL) {
	case (TOK_IDENT): case (TOK_RESTART): case (TOK_STRSTART):
		{
			p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI111, ZI112, ZI113);
			/* BEGINNING OF INLINE: 117 */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_117;
			}
			/* END OF INLINE: 117 */
		}
		/*UNREACHED*/
	default:
		{
			ZI114 = ZI111;
			ZI115 = ZI112;
			ZI116 = ZI113;
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
	*ZO114 = ZI114;
	*ZO115 = ZI115;
	*ZO116 = ZI116;
}

static void
p_123(lex_state lex_state, act_state act_state, zone ZI119, fsm ZI120, zone *ZO121, fsm *ZO122)
{
	zone ZI121;
	fsm ZI122;

ZL2_123:;
	switch (CURRENT_TERMINAL) {
	case (TOK_IDENT): case (TOK_RESTART): case (TOK_STRSTART):
		{
			fsm ZIb;
			fsm ZIq;

			p_concaternated_Hpatterns_C_Cpattern (lex_state, act_state, ZI119, &ZIb);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-concat */
			{
#line 344 "parser.act"

		assert((ZI120) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_concat((ZI120), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_concat");
			goto ZL1;
		}
	
#line 1450 "parser.c"
			}
			/* END OF ACTION: op-concat */
			/* BEGINNING OF INLINE: 123 */
			ZI120 = ZIq;
			goto ZL2_123;
			/* END OF INLINE: 123 */
		}
		/*UNREACHED*/
	default:
		{
			ZI121 = ZI119;
			ZI122 = ZI120;
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
	*ZO121 = ZI121;
	*ZO122 = ZI122;
}

static void
p_126(lex_state lex_state, act_state act_state, ast *ZIa, zone *ZIz, fsm *ZIexit, string *ZIn)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_BIND):
		{
			fsm ZIr;
			fsm ZIq;

			/* BEGINNING OF INLINE: 81 */
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
#line 408 "parser.act"

		err_expected("'='");
	
#line 1506 "parser.c"
					}
					/* END OF ACTION: err-expected-bind */
				}
			ZL2:;
			}
			/* END OF INLINE: 81 */
			p_concaternated_Hpatterns (lex_state, act_state, *ZIz, &ZIr);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 355 "parser.act"

		assert((ZIr) != NULL);

		if ((*ZIexit) == NULL) {
			(ZIq) = (ZIr);
		} else {
			(*ZIexit) = fsm_clone((*ZIexit));
			if ((*ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZIq) = fsm_subtract((ZIr), (*ZIexit));
			if ((ZIq) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if (!fsm_trim((ZIq))) {
				perror("fsm_trim");
				goto ZL1;
			}
		}
	
#line 1545 "parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_97 (lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: add-binding */
			{
#line 281 "parser.act"

		struct var *v;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIn) != NULL);
		assert((ZIq) != NULL);

		(void) (*ZIa);

		v = var_bind(&(*ZIz)->vl, (*ZIn), (ZIq));
		if (v == NULL) {
			perror("var_bind");
			goto ZL1;
		}
	
#line 1572 "parser.c"
			}
			/* END OF ACTION: add-binding */
		}
		break;
	case (TOK_IDENT): case (TOK_RESTART): case (TOK_STRSTART): case (TOK_SEMI):
	case (TOK_TO): case (TOK_MAP):
		{
			fsm ZIq;
			zone ZI137;
			fsm ZI138;
			string ZI139;
			fsm ZI140;

			/* BEGINNING OF ACTION: deref-var */
			{
#line 158 "parser.act"

		assert((*ZIn) != NULL);

		/* TODO: walk zone tree to root */

		(ZIq) = var_find((*ZIz)->vl, (*ZIn));
		if ((ZIq) == NULL) {
			/* TODO: use *err */
			fprintf(stderr, "No such variable: %s\n", (*ZIn));
			goto ZL1;
		}

		(ZIq) = fsm_clone((ZIq));
		if ((ZIq) == NULL) {
			/* TODO: use *err */
			perror("fsm_clone");
			goto ZL1;
		}
	
#line 1608 "parser.c"
			}
			/* END OF ACTION: deref-var */
			p_123 (lex_state, act_state, *ZIz, ZIq, &ZI137, &ZI138);
			p_75 (lex_state, act_state, &ZI139);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: subtract-exit */
			{
#line 355 "parser.act"

		assert((ZI138) != NULL);

		if ((*ZIexit) == NULL) {
			(ZI140) = (ZI138);
		} else {
			(*ZIexit) = fsm_clone((*ZIexit));
			if ((*ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI140) = fsm_subtract((ZI138), (*ZIexit));
			if ((ZI140) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if (!fsm_trim((ZI140))) {
				perror("fsm_trim");
				goto ZL1;
			}
		}
	
#line 1644 "parser.c"
			}
			/* END OF ACTION: subtract-exit */
			p_141 (lex_state, act_state, ZIa, ZIz, &ZI139, &ZI140);
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

/* BEGINNING OF TRAILER */

#line 477 "parser.act"


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

#line 1711 "parser.c"

/* END OF FILE */
