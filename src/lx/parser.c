/*
 * Automatically generated from the files:
 *	parser.sid
 * and
 *	parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 79 "parser.act"


	#include <assert.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <errno.h>

	#include <adt/xalloc.h>

	#include <fsm/fsm.h>
	#include <fsm/bool.h>
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

	static int notend(const struct fsm *fsm, const struct fsm_state *state) {
		return !fsm_isend(fsm, state);
	}

	static int trapstate(const struct fsm *fsm, const struct fsm_state *state) {
		if (state == fsm_getstart(fsm)) {
			return 0;
		}

		if (fsm_isend(fsm, state)) {
			return 0;
		}

		return 1;
	}

#line 73 "parser.c"

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Hthings(lex_state, act_state, ast, zone);
static void p_list_Hof_Hthings_C_Cthing(lex_state, act_state, ast, zone);
static void p_token_Hmapping_C_Cpattern(lex_state, act_state, fsm *);
static void p_token_Hmapping(lex_state, act_state, fsm *, string *);
static void p_token_Hmapping_C_Cconcaternated_Hpatterns(lex_state, act_state, fsm *);
static void p_69(lex_state, act_state);
static void p_84(lex_state, act_state, ast, zone, ast *, zone *);
static void p_88(lex_state, act_state, fsm, fsm *);
static void p_92(lex_state, act_state, ast *, zone *, fsm *, string *);
extern void p_lx(lex_state, act_state, ast *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Hthings(lex_state lex_state, act_state act_state, ast ZI80, zone ZI81)
{
	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		ast ZI82;
		zone ZI83;

		p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI80, ZI81);
		p_84 (lex_state, act_state, ZI80, ZI81, &ZI82, &ZI83);
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
#line 334 "parser.act"

		err_expected("list of token mappings or zones");
	
#line 119 "parser.c"
		}
		/* END OF ACTION: err-expected-list */
	}
}

static void
p_list_Hof_Hthings_C_Cthing(lex_state lex_state, act_state act_state, ast ZIa, zone ZIz)
{
	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		fsm ZI90;
		string ZI91;

		p_token_Hmapping (lex_state, act_state, &ZI90, &ZI91);
		p_92 (lex_state, act_state, &ZIa, &ZIz, &ZI90, &ZI91);
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
#line 104 "parser.act"

		ZIs = xstrdup(lex_tokbuf(lex_state));
		if (ZIs == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 168 "parser.c"
			}
			/* END OF EXTRACT: pattern_literal */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: compile-literal */
			{
#line 122 "parser.act"

		assert((ZIs) != NULL);

		/* TODO: handle error codes */
		(ZIr) = re_new_comp(RE_LITERAL, re_getc_str, &(ZIs), 0, NULL);
		if ((ZIr) == NULL) {
			/* TODO: use *err */
			goto ZL1;
		}
	
#line 185 "parser.c"
			}
			/* END OF ACTION: compile-literal */
		}
		break;
	case 0:
		{
			string ZIs;

			/* BEGINNING OF EXTRACT: pattern_regex */
			{
#line 96 "parser.act"

		ZIs = xstrdup(lex_tokbuf(lex_state));
		if (ZIs == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 204 "parser.c"
			}
			/* END OF EXTRACT: pattern_regex */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: compile-regex */
			{
#line 133 "parser.act"

		assert((ZIs) != NULL);

		/* TODO: handle error codes */
		(ZIr) = re_new_comp(RE_SIMPLE, re_getc_str, &(ZIs), 0, NULL);
		if ((ZIr) == NULL) {
			/* TODO: use *err */
			goto ZL1;
		}
	
#line 221 "parser.c"
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
		/* BEGINNING OF INLINE: 52 */
		{
			switch (CURRENT_TERMINAL) {
			case 2:
				{
					/* BEGINNING OF INLINE: 53 */
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
#line 310 "parser.act"

		err_expected("'->'");
	
#line 275 "parser.c"
							}
							/* END OF ACTION: err-expected-map */
						}
					ZL3:;
					}
					/* END OF INLINE: 53 */
					switch (CURRENT_TERMINAL) {
					case 6:
						/* BEGINNING OF EXTRACT: token_name */
						{
#line 112 "parser.act"

		ZIt = xstrdup(lex_tokbuf(lex_state));
		if (ZIt == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 294 "parser.c"
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
#line 206 "parser.act"

		(ZIt) = NULL;
	
#line 312 "parser.c"
					}
					/* END OF ACTION: no-token */
				}
				break;
			case 11:
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 52 */
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
p_token_Hmapping_C_Cconcaternated_Hpatterns(lex_state lex_state, act_state act_state, fsm *ZO87)
{
	fsm ZI87;

	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		fsm ZIq;

		p_token_Hmapping_C_Cpattern (lex_state, act_state, &ZIq);
		p_88 (lex_state, act_state, ZIq, &ZI87);
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
	*ZO87 = ZI87;
}

static void
p_69(lex_state lex_state, act_state act_state)
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
#line 314 "parser.act"

		err_expected("';'");
	
#line 383 "parser.c"
		}
		/* END OF ACTION: err-expected-semi */
	}
}

static void
p_84(lex_state lex_state, act_state act_state, ast ZI80, zone ZI81, ast *ZO82, zone *ZO83)
{
	ast ZI82;
	zone ZI83;

ZL2_84:;
	switch (CURRENT_TERMINAL) {
	case 0: case 1:
		{
			p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI80, ZI81);
			/* BEGINNING OF INLINE: 84 */
			if ((CURRENT_TERMINAL) == 11) {
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
			ZI82 = ZI80;
			ZI83 = ZI81;
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
	*ZO82 = ZI82;
	*ZO83 = ZI83;
}

static void
p_88(lex_state lex_state, act_state act_state, fsm ZI86, fsm *ZO87)
{
	fsm ZI87;

ZL2_88:;
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
#line 275 "parser.act"

		assert((ZI86) != NULL);
		assert((ZIb) != NULL);

		(ZIq) = fsm_concat((ZI86), (ZIb));
		if ((ZIq) == NULL) {
			perror("fsm_concat");
			goto ZL1;
		}
	
#line 458 "parser.c"
			}
			/* END OF ACTION: op-concat */
			/* BEGINNING OF INLINE: 88 */
			ZI86 = ZIq;
			goto ZL2_88;
			/* END OF INLINE: 88 */
		}
		/*UNREACHED*/
	default:
		{
			ZI87 = ZI86;
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
	*ZO87 = ZI87;
}

static void
p_92(lex_state lex_state, act_state act_state, ast *ZIa, zone *ZIz, fsm *ZI90, string *ZI91)
{
	switch (CURRENT_TERMINAL) {
	case 3:
		{
			fsm ZIr2;
			string ZIt2;
			zone ZIchild;

			/* BEGINNING OF INLINE: 64 */
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
#line 318 "parser.act"

		err_expected("'..'");
	
#line 513 "parser.c"
					}
					/* END OF ACTION: err-expected-to */
				}
			ZL2:;
			}
			/* END OF INLINE: 64 */
			p_token_Hmapping (lex_state, act_state, &ZIr2, &ZIt2);
			if ((CURRENT_TERMINAL) == 11) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: make-zone */
			{
#line 163 "parser.act"

		assert((*ZIa) != NULL);

		(ZIchild) = ast_addzone((*ZIa));
		if ((ZIchild) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 537 "parser.c"
			}
			/* END OF ACTION: make-zone */
			/* BEGINNING OF INLINE: 68 */
			{
				switch (CURRENT_TERMINAL) {
				case 4:
					{
						zone ZIx;
						string ZIy;
						fsm ZIv;

						p_69 (lex_state, act_state);
						if ((CURRENT_TERMINAL) == 11) {
							RESTORE_LEXER;
							goto ZL5;
						}
						/* BEGINNING OF ACTION: no-to */
						{
#line 210 "parser.act"

		(ZIx) = NULL;
	
#line 560 "parser.c"
						}
						/* END OF ACTION: no-to */
						/* BEGINNING OF ACTION: no-token */
						{
#line 206 "parser.act"

		(ZIy) = NULL;
	
#line 569 "parser.c"
						}
						/* END OF ACTION: no-token */
						/* BEGINNING OF ACTION: clone */
						{
#line 286 "parser.act"

		assert((ZIr2) != NULL);

		(ZIv) = fsm_clone((ZIr2));
		if ((ZIv) == NULL) {
			perror("fsm_clone");
			goto ZL5;
		}
	
#line 584 "parser.c"
						}
						/* END OF ACTION: clone */
						/* BEGINNING OF ACTION: trap */
						{
#line 217 "parser.act"

		struct fsm_state *trap;
		struct fsm_state *s;

		assert((ZIv) != NULL);

		if (!fsm_complete((ZIv), notend)) {
			perror("fsm_complete");
			goto ZL5;
		}

		trap = fsm_collate((ZIv), trapstate);
		if (trap == NULL && errno != 0) { /* XXX: hackish */
			if (trap == NULL) {
				perror("fsm_collate");
				goto ZL5;
			}
		}

		for (s = (ZIv)->sl; s != NULL; s = s->next) {
			fsm_setend((ZIv), s, 0);
		}

		if (trap != NULL) {
			assert(trap != fsm_getstart((ZIv)));

			fsm_setend((ZIv), trap, 1);

			/* TODO: must set opaque here, i think */
		}
	
#line 621 "parser.c"
						}
						/* END OF ACTION: trap */
						/* BEGINNING OF ACTION: make-mapping */
						{
#line 176 "parser.act"

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
	
#line 652 "parser.c"
						}
						/* END OF ACTION: make-mapping */
					}
					break;
				case 7:
					{
						/* BEGINNING OF INLINE: 73 */
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
#line 322 "parser.act"

		err_expected("'{'");
	
#line 679 "parser.c"
								}
								/* END OF ACTION: err-expected-open */
							}
						ZL6:;
						}
						/* END OF INLINE: 73 */
						p_list_Hof_Hthings (lex_state, act_state, *ZIa, ZIchild);
						/* BEGINNING OF INLINE: 74 */
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
#line 326 "parser.act"

		err_expected("'}'");
	
#line 711 "parser.c"
								}
								/* END OF ACTION: err-expected-close */
							}
						ZL8:;
						}
						/* END OF INLINE: 74 */
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
#line 334 "parser.act"

		err_expected("list of token mappings or zones");
	
#line 732 "parser.c"
					}
					/* END OF ACTION: err-expected-list */
				}
			ZL4:;
			}
			/* END OF INLINE: 68 */
			/* BEGINNING OF ACTION: make-mapping */
			{
#line 176 "parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIchild));
		assert((*ZI90) != NULL);

		if ((*ZI91) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI91));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI90), t, (ZIchild));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 767 "parser.c"
			}
			/* END OF ACTION: make-mapping */
			/* BEGINNING OF ACTION: make-mapping */
			{
#line 176 "parser.act"

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
	
#line 798 "parser.c"
			}
			/* END OF ACTION: make-mapping */
		}
		break;
	case 4:
		{
			zone ZIto;

			p_69 (lex_state, act_state);
			if ((CURRENT_TERMINAL) == 11) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: no-to */
			{
#line 210 "parser.act"

		(ZIto) = NULL;
	
#line 818 "parser.c"
			}
			/* END OF ACTION: no-to */
			/* BEGINNING OF ACTION: make-mapping */
			{
#line 176 "parser.act"

		struct ast_token *t;
		struct ast_mapping *m;

		assert((*ZIa) != NULL);
		assert((*ZIz) != NULL);
		assert((*ZIz) != (ZIto));
		assert((*ZI90) != NULL);

		if ((*ZI91) != NULL) {
			t = ast_addtoken((*ZIa), (*ZI91));
			if (t == NULL) {
				perror("ast_addtoken");
				goto ZL1;
			}
		} else {
			t = NULL;
		}

		m = ast_addmapping((*ZIz), (*ZI90), t, (ZIto));
		if (m == NULL) {
			perror("ast_addmapping");
			goto ZL1;
		}
	
#line 849 "parser.c"
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

void
p_lx(lex_state lex_state, act_state act_state, ast *ZOa)
{
	ast ZIa;

	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		zone ZIz;

		/* BEGINNING OF ACTION: make-ast */
		{
#line 155 "parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL1;
		}
	
#line 886 "parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: make-zone */
		{
#line 163 "parser.act"

		assert((ZIa) != NULL);

		(ZIz) = ast_addzone((ZIa));
		if ((ZIz) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 901 "parser.c"
		}
		/* END OF ACTION: make-zone */
		/* BEGINNING OF ACTION: set-globalzone */
		{
#line 199 "parser.act"

		assert((ZIa) != NULL);
		assert((ZIz) != NULL);

		(ZIa)->global = (ZIz);
	
#line 913 "parser.c"
		}
		/* END OF ACTION: set-globalzone */
		p_list_Hof_Hthings (lex_state, act_state, ZIa, ZIz);
		/* BEGINNING OF INLINE: 78 */
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
#line 330 "parser.act"

		err_expected("EOF");
	
#line 941 "parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL2:;
		}
		/* END OF INLINE: 78 */
	}
	goto ZL0;
ZL1:;
	{
		/* BEGINNING OF ACTION: make-ast */
		{
#line 155 "parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL4;
		}
	
#line 962 "parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: err-syntax */
		{
#line 302 "parser.act"

		fprintf(stderr, "syntax error\n");
		exit(EXIT_FAILURE);
	
#line 972 "parser.c"
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

/* BEGINNING OF TRAILER */

#line 365 "parser.act"


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

#line 1016 "parser.c"

/* END OF FILE */
