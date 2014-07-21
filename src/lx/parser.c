/*
 * Automatically generated from the files:
 *	parser.sid
 * and
 *	parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 64 "parser.act"


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
		int lex_tok;
		int lex_tok_save;
	};

	#define CURRENT_TERMINAL (act_state->lex_tok)
	#define ERROR_TERMINAL   lex_unknown
	#define ADVANCE_LEXER    do { act_state->lex_tok = lex_nexttoken(lex_state); } while (0)
	#define SAVE_LEXER(tok)  do { act_state->lex_tok_save = act_state->lex_tok; \
	                              act_state->lex_tok = tok;                      } while (0)
	#define RESTORE_LEXER    do { act_state->lex_tok = act_state->lex_tok_save;  } while (0)

	static void err_expected(const char *token) {
		fprintf(stderr, "syntax error: expected %s\n", token);
		exit(EXIT_FAILURE);
	}

#line 58 "parser.c"

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Hthings(lex_state, act_state, ast, zone, fsm);
static void p_list_Hof_Hthings_C_Cthing(lex_state, act_state, ast, zone, fsm);
static void p_token_Hmapping_C_Cpattern(lex_state, act_state, fsm *);
static void p_token_Hmapping(lex_state, act_state, fsm *, string *);
static void p_token_Hmapping_C_Cconcaternated_Hpatterns(lex_state, act_state, fsm *);
static void p_73(lex_state, act_state);
static void p_92(lex_state, act_state, ast, zone, fsm, ast *, zone *, fsm *);
static void p_96(lex_state, act_state, fsm, fsm *);
extern void p_lx(lex_state, act_state, ast *);
static void p_102(lex_state, act_state, ast *, zone *, string *, fsm *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Hthings(lex_state lex_state, act_state act_state, ast ZI86, zone ZI87, fsm ZI88)
{
	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		ast ZI89;
		zone ZI90;
		fsm ZI91;

		p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI86, ZI87, ZI88);
		p_92 (lex_state, act_state, ZI86, ZI87, ZI88, &ZI89, &ZI90, &ZI91);
		if ((CURRENT_TERMINAL) == 11) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-list */
		{
#line 340 "parser.act"

		err_expected("list of token mappings or zones");
	
#line 105 "parser.c"
		}
		/* END OF ACTION: err-expected-list */
	}
}

static void
p_list_Hof_Hthings_C_Cthing(lex_state lex_state, act_state act_state, ast ZIa, zone ZIz, fsm ZIexit)
{
	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		fsm ZI99;
		string ZI100;
		fsm ZI101;

		p_token_Hmapping (lex_state, act_state, &ZI99, &ZI100);
		if ((CURRENT_TERMINAL) == 11) {
			RESTORE_LEXER;
			goto ZL1;
		}
		/* BEGINNING OF ACTION: subtract-exit */
		{
#line 267 "parser.act"

		assert((ZI99) != NULL);

		if ((ZIexit) == NULL) {
			(ZI101) = (ZI99);
		} else {
			(ZIexit) = fsm_clone((ZIexit));
			if ((ZIexit) == NULL) {
				perror("fsm_clone");
				goto ZL1;
			}

			(ZI101) = fsm_subtract((ZI99), (ZIexit));
			if ((ZI101) == NULL) {
				perror("fsm_subtract");
				goto ZL1;
			}

			if (!fsm_trim((ZI101))) {
				perror("fsm_trim");
				goto ZL1;
			}
		}
	
#line 154 "parser.c"
		}
		/* END OF ACTION: subtract-exit */
		p_102 (lex_state, act_state, &ZIa, &ZIz, &ZI100, &ZI101);
		if ((CURRENT_TERMINAL) == 11) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	return;
ZL1:;
	SAVE_LEXER (11);
	return;
}

static void
p_token_Hmapping_C_Cpattern(lex_state lex_state, act_state act_state, fsm *ZOr)
{
	fsm ZIr;

	switch (CURRENT_TERMINAL) {
	case 1:
		{
			string ZIs;

			/* BEGINNING OF EXTRACT: pattern_literal */
			{
#line 89 "parser.act"

		ZIs = xstrdup(lex_tokbuf(lex_state));
		if (ZIs == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 189 "parser.c"
			}
			/* END OF EXTRACT: pattern_literal */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: compile-literal */
			{
#line 107 "parser.act"

		assert((ZIs) != NULL);

		/* TODO: handle error codes */
		(ZIr) = re_new_comp(RE_LITERAL, re_getc_str, &(ZIs), 0, NULL);
		if ((ZIr) == NULL) {
			/* TODO: use *err */
			goto ZL1;
		}
	
#line 206 "parser.c"
			}
			/* END OF ACTION: compile-literal */
		}
		break;
	case 0:
		{
			string ZIs;

			/* BEGINNING OF EXTRACT: pattern_regex */
			{
#line 81 "parser.act"

		ZIs = xstrdup(lex_tokbuf(lex_state));
		if (ZIs == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 225 "parser.c"
			}
			/* END OF EXTRACT: pattern_regex */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: compile-regex */
			{
#line 118 "parser.act"

		assert((ZIs) != NULL);

		/* TODO: handle error codes */
		(ZIr) = re_new_comp(RE_SIMPLE, re_getc_str, &(ZIs), 0, NULL);
		if ((ZIr) == NULL) {
			/* TODO: use *err */
			goto ZL1;
		}
	
#line 242 "parser.c"
			}
			/* END OF ACTION: compile-regex */
		}
		break;
	case 11:
		return;
	default:
		goto ZL1;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER (11);
	return;
ZL0:;
	*ZOr = ZIr;
}

static void
p_token_Hmapping(lex_state lex_state, act_state act_state, fsm *ZOr, string *ZOt)
{
	fsm ZIr;
	string ZIt;

	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		p_token_Hmapping_C_Cconcaternated_Hpatterns (lex_state, act_state, &ZIr);
		/* BEGINNING OF INLINE: 54 */
		{
			switch (CURRENT_TERMINAL) {
			case 2:
				{
					/* BEGINNING OF INLINE: 55 */
					{
						{
							switch (CURRENT_TERMINAL) {
							case 2:
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
#line 316 "parser.act"

		err_expected("'->'");
	
#line 296 "parser.c"
							}
							/* END OF ACTION: err-expected-map */
						}
					ZL3:;
					}
					/* END OF INLINE: 55 */
					switch (CURRENT_TERMINAL) {
					case 6:
						/* BEGINNING OF EXTRACT: token_name */
						{
#line 97 "parser.act"

		ZIt = xstrdup(lex_tokbuf(lex_state));
		if (ZIt == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 315 "parser.c"
						}
						/* END OF EXTRACT: token_name */
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
#line 215 "parser.act"

		(ZIt) = NULL;
	
#line 333 "parser.c"
					}
					/* END OF ACTION: no-token */
				}
				break;
			case 11:
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 54 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER (11);
	return;
ZL0:;
	*ZOr = ZIr;
	*ZOt = ZIt;
}

static void
p_token_Hmapping_C_Cconcaternated_Hpatterns(lex_state lex_state, act_state act_state, fsm *ZO95)
{
	fsm ZI95;

	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		fsm ZIq;

		p_token_Hmapping_C_Cpattern (lex_state, act_state, &ZIq);
		p_96 (lex_state, act_state, ZIq, &ZI95);
		if ((CURRENT_TERMINAL) == 11) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER (11);
	return;
ZL0:;
	*ZO95 = ZI95;
}

static void
p_73(lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case 4:
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
#line 320 "parser.act"

		err_expected("';'");
	
#line 404 "parser.c"
		}
		/* END OF ACTION: err-expected-semi */
	}
}

static void
p_92(lex_state lex_state, act_state act_state, ast ZI86, zone ZI87, fsm ZI88, ast *ZO89, zone *ZO90, fsm *ZO91)
{
	ast ZI89;
	zone ZI90;
	fsm ZI91;

ZL2_92:;
	switch (CURRENT_TERMINAL) {
	case 0: case 1:
		{
			p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI86, ZI87, ZI88);
			/* BEGINNING OF INLINE: 92 */
			if ((CURRENT_TERMINAL) == 11) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_92;
			}
			/* END OF INLINE: 92 */
		}
		/*UNREACHED*/
	default:
		{
			ZI89 = ZI86;
			ZI90 = ZI87;
			ZI91 = ZI88;
		}
		break;
	case 11:
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER (11);
	return;
ZL0:;
	*ZO89 = ZI89;
	*ZO90 = ZI90;
	*ZO91 = ZI91;
}

static void
p_96(lex_state lex_state, act_state act_state, fsm ZI94, fsm *ZO95)
{
	fsm ZI95;

ZL2_96:;
	switch (CURRENT_TERMINAL) {
	case 0: case 1:
		{
			fsm ZIb;
			fsm ZIq;

			p_token_Hmapping_C_Cpattern (lex_state, act_state, &ZIb);
			if ((CURRENT_TERMINAL) == 11) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: op-concat */
			{
#line 256 "parser.act"

		assert((ZI94) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_concat((ZI94), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_concat");
			goto ZL1;
		}
	
#line 482 "parser.c"
			}
			/* END OF ACTION: op-concat */
			/* BEGINNING OF INLINE: 96 */
			ZI94 = ZIq;
			goto ZL2_96;
			/* END OF INLINE: 96 */
		}
		/*UNREACHED*/
	default:
		{
			ZI95 = ZI94;
		}
		break;
	case 11:
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER (11);
	return;
ZL0:;
	*ZO95 = ZI95;
}

void
p_lx(lex_state lex_state, act_state act_state, ast *ZOa)
{
	ast ZIa;

	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		zone ZIz;
		fsm ZIexit;

		/* BEGINNING OF ACTION: make-ast */
		{
#line 164 "parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL1;
		}
	
#line 529 "parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: make-zone */
		{
#line 172 "parser.act"

		assert((ZIa) != NULL);

		(ZIz) = ast_addzone((ZIa));
		if ((ZIz) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 544 "parser.c"
		}
		/* END OF ACTION: make-zone */
		/* BEGINNING OF ACTION: no-exit */
		{
#line 219 "parser.act"

		(ZIexit) = NULL;
	
#line 553 "parser.c"
		}
		/* END OF ACTION: no-exit */
		/* BEGINNING OF ACTION: set-globalzone */
		{
#line 208 "parser.act"

		assert((ZIa) != NULL);
		assert((ZIz) != NULL);

		(ZIa)->global = (ZIz);
	
#line 565 "parser.c"
		}
		/* END OF ACTION: set-globalzone */
		p_list_Hof_Hthings (lex_state, act_state, ZIa, ZIz, ZIexit);
		/* BEGINNING OF INLINE: 84 */
		{
			if ((CURRENT_TERMINAL) == 11) {
				RESTORE_LEXER;
				goto ZL1;
			}
			{
				switch (CURRENT_TERMINAL) {
				case 10:
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
#line 336 "parser.act"

		err_expected("EOF");
	
#line 593 "parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL2:;
		}
		/* END OF INLINE: 84 */
	}
	goto ZL0;
ZL1:;
	{
		/* BEGINNING OF ACTION: make-ast */
		{
#line 164 "parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL4;
		}
	
#line 614 "parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: err-syntax */
		{
#line 308 "parser.act"

		fprintf(stderr, "syntax error\n");
		exit(EXIT_FAILURE);
	
#line 624 "parser.c"
		}
		/* END OF ACTION: err-syntax */
	}
	goto ZL0;
ZL4:;
	SAVE_LEXER (11);
	return;
ZL0:;
	*ZOa = ZIa;
}

static void
p_102(lex_state lex_state, act_state act_state, ast *ZIa, zone *ZIz, string *ZI100, fsm *ZI101)
{
	switch (CURRENT_TERMINAL) {
	case 3:
		{
			fsm ZIr2;
			string ZIt2;
			zone ZIchild;

			/* BEGINNING OF INLINE: 68 */
			{
				{
					switch (CURRENT_TERMINAL) {
					case 3:
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
#line 324 "parser.act"

		err_expected("'..'");
	
#line 666 "parser.c"
					}
					/* END OF ACTION: err-expected-to */
				}
			ZL2:;
			}
			/* END OF INLINE: 68 */
			p_token_Hmapping (lex_state, act_state, &ZIr2, &ZIt2);
			if ((CURRENT_TERMINAL) == 11) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: make-zone */
			{
#line 172 "parser.act"

		assert((*ZIa) != NULL);

		(ZIchild) = ast_addzone((*ZIa));
		if ((ZIchild) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 690 "parser.c"
			}
			/* END OF ACTION: make-zone */
			/* BEGINNING OF INLINE: 72 */
			{
				switch (CURRENT_TERMINAL) {
				case 4:
					{
						zone ZIx;
						string ZIy;
						fsm ZIu;
						fsm ZIw;
						fsm ZIv;

						p_73 (lex_state, act_state);
						if ((CURRENT_TERMINAL) == 11) {
							RESTORE_LEXER;
							goto ZL5;
						}
						/* BEGINNING OF ACTION: no-to */
						{
#line 223 "parser.act"

		(ZIx) = NULL;
	
#line 715 "parser.c"
						}
						/* END OF ACTION: no-to */
						/* BEGINNING OF ACTION: no-token */
						{
#line 215 "parser.act"

		(ZIy) = NULL;
	
#line 724 "parser.c"
						}
						/* END OF ACTION: no-token */
						/* BEGINNING OF ACTION: clone */
						{
#line 292 "parser.act"

		assert((ZIr2) != NULL);

		(ZIu) = fsm_clone((ZIr2));
		if ((ZIu) == NULL) {
			perror("fsm_clone");
			goto ZL5;
		}
	
#line 739 "parser.c"
						}
						/* END OF ACTION: clone */
						/* BEGINNING OF ACTION: regex-any */
						{
#line 143 "parser.act"

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
	
#line 767 "parser.c"
						}
						/* END OF ACTION: regex-any */
						/* BEGINNING OF ACTION: subtract-exit */
						{
#line 267 "parser.act"

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
	
#line 797 "parser.c"
						}
						/* END OF ACTION: subtract-exit */
						/* BEGINNING OF ACTION: make-mapping */
						{
#line 185 "parser.act"

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
	
#line 828 "parser.c"
						}
						/* END OF ACTION: make-mapping */
					}
					break;
				case 7:
					{
						/* BEGINNING OF INLINE: 79 */
						{
							{
								switch (CURRENT_TERMINAL) {
								case 7:
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
#line 328 "parser.act"

		err_expected("'{'");
	
#line 855 "parser.c"
								}
								/* END OF ACTION: err-expected-open */
							}
						ZL6:;
						}
						/* END OF INLINE: 79 */
						p_list_Hof_Hthings (lex_state, act_state, *ZIa, ZIchild, ZIr2);
						/* BEGINNING OF INLINE: 80 */
						{
							if ((CURRENT_TERMINAL) == 11) {
								RESTORE_LEXER;
								goto ZL5;
							}
							{
								switch (CURRENT_TERMINAL) {
								case 8:
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
#line 332 "parser.act"

		err_expected("'}'");
	
#line 887 "parser.c"
								}
								/* END OF ACTION: err-expected-close */
							}
						ZL8:;
						}
						/* END OF INLINE: 80 */
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
#line 340 "parser.act"

		err_expected("list of token mappings or zones");
	
#line 908 "parser.c"
					}
					/* END OF ACTION: err-expected-list */
				}
			ZL4:;
			}
			/* END OF INLINE: 72 */
			/* BEGINNING OF ACTION: make-mapping */
			{
#line 185 "parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIchild));
		assert((*ZI101) != NULL);

		if ((*ZI100) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI100));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI101), t, (ZIchild));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 943 "parser.c"
			}
			/* END OF ACTION: make-mapping */
			/* BEGINNING OF ACTION: make-mapping */
			{
#line 185 "parser.act"

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
	
#line 974 "parser.c"
			}
			/* END OF ACTION: make-mapping */
		}
		break;
	case 4:
		{
			zone ZIto;

			p_73 (lex_state, act_state);
			if ((CURRENT_TERMINAL) == 11) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: no-to */
			{
#line 223 "parser.act"

		(ZIto) = NULL;
	
#line 994 "parser.c"
			}
			/* END OF ACTION: no-to */
			/* BEGINNING OF ACTION: make-mapping */
			{
#line 185 "parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIto));
		assert((*ZI101) != NULL);

		if ((*ZI100) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI100));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI101), t, (ZIto));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 1025 "parser.c"
			}
			/* END OF ACTION: make-mapping */
		}
		break;
	case 11:
		return;
	default:
		goto ZL1;
	}
	return;
ZL1:;
	SAVE_LEXER (11);
	return;
}

/* BEGINNING OF TRAILER */

#line 371 "parser.act"


	struct ast *lx_parse(FILE *f) {
		struct act_state act_state_s;
		struct act_state *act_state;
		struct lex_state *lex_state;
		struct ast *ast;

		assert(f != NULL);

		lex_state = lex_init(f);
		if (lex_state == NULL) {
			perror("lex_init");
			return NULL;
		}

		/* This is a workaround for ADVANCE_LEXER assuming a pointer */
		act_state = &act_state_s;

		ADVANCE_LEXER;	/* XXX: what if the first token is unrecognised? */
		p_lx(lex_state, act_state, &ast);

		assert(ast != NULL);

		lex_free(lex_state);

		return ast;
	}

#line 1073 "parser.c"

/* END OF FILE */
