/*
 * Automatically generated from the files:
 *	parser.sid
 * and
 *	parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 54 "parser.act"


	#include <assert.h>
	#include <stdio.h>
	#include <stdlib.h>

	#include <re/re.h>

	#include "lexer.h"
	#include "parser.h"
	#include "xalloc.h"
	#include "ast.h"

	typedef char *           string;
	typedef struct re *      re;
	typedef struct lx_zone * zone;

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

#line 48 "parser.c"

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Hthings(lex_state, act_state, ast, zone);
static void p_list_Hof_Hthings_C_Cthing(lex_state, act_state, ast, zone);
static void p_56(lex_state, act_state);
static void p_token_Hmapping_C_Cpattern(lex_state, act_state, re *);
static void p_67(lex_state, act_state, ast, zone, ast *, zone *);
static void p_token_Hmapping(lex_state, act_state, zone);
static void p_token_Hmapping_C_Cconcaternated_Hpatterns(lex_state, act_state, re *);
static void p_71(lex_state, act_state, re, re *);
static void p_73(lex_state, act_state, ast *);
extern void p_lx(lex_state, act_state, ast *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Hthings(lex_state lex_state, act_state act_state, ast ZI63, zone ZI64)
{
	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		ast ZI65;
		zone ZI66;

		p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI63, ZI64);
		p_67 (lex_state, act_state, ZI63, ZI64, &ZI65, &ZI66);
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
#line 205 "parser.act"

		err_expected("list of token mappings or zones");
	
#line 94 "parser.c"
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
		p_token_Hmapping (lex_state, act_state, ZIz);
		p_73 (lex_state, act_state, &ZIa);
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
p_56(lex_state lex_state, act_state act_state)
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
#line 181 "parser.act"

		err_expected("';'");
	
#line 144 "parser.c"
		}
		/* END OF ACTION: err-expected-semi */
	}
}

static void
p_token_Hmapping_C_Cpattern(lex_state lex_state, act_state act_state, re *ZOr)
{
	re ZIr;

	switch (CURRENT_TERMINAL) {
	case 1:
		{
			string ZIs;

			/* BEGINNING OF EXTRACT: pattern_literal */
			{
#line 79 "parser.act"

		ZIs = xstrdup(lex_tokbuf(lex_state));
		if (ZIs == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 170 "parser.c"
			}
			/* END OF EXTRACT: pattern_literal */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: compile-literal */
			{
#line 97 "parser.act"

		assert((ZIs) != NULL);

		(ZIr) = re_new_comp(RE_LITERAL, re_getc_str, &(ZIs), 0, NULL, NULL);
		if ((ZIr) == NULL) {
			/* TODO: use *err */
			goto ZL1;
		}
	
#line 186 "parser.c"
			}
			/* END OF ACTION: compile-literal */
		}
		break;
	case 0:
		{
			string ZIs;

			/* BEGINNING OF EXTRACT: pattern_regex */
			{
#line 71 "parser.act"

		ZIs = xstrdup(lex_tokbuf(lex_state));
		if (ZIs == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 205 "parser.c"
			}
			/* END OF EXTRACT: pattern_regex */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: compile-regex */
			{
#line 107 "parser.act"

		assert((ZIs) != NULL);

		(ZIr) = re_new_comp(RE_SIMPLE, re_getc_str, &(ZIs), 0, NULL, NULL);
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
p_67(lex_state lex_state, act_state act_state, ast ZI63, zone ZI64, ast *ZO65, zone *ZO66)
{
	ast ZI65;
	zone ZI66;

ZL2_67:;
	switch (CURRENT_TERMINAL) {
	case 0: case 1:
		{
			p_list_Hof_Hthings_C_Cthing (lex_state, act_state, ZI63, ZI64);
			/* BEGINNING OF INLINE: 67 */
			if ((CURRENT_TERMINAL) == 11) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_67;
			}
			/* END OF INLINE: 67 */
		}
		/*UNREACHED*/
	default:
		{
			ZI65 = ZI63;
			ZI66 = ZI64;
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
	*ZO65 = ZI65;
	*ZO66 = ZI66;
}

static void
p_token_Hmapping(lex_state lex_state, act_state act_state, zone ZIz)
{
	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		re ZIr;

		p_token_Hmapping_C_Cconcaternated_Hpatterns (lex_state, act_state, &ZIr);
		/* BEGINNING OF INLINE: 44 */
		{
			switch (CURRENT_TERMINAL) {
			case 2:
				{
					string ZIt;

					/* BEGINNING OF INLINE: 46 */
					{
						{
							switch (CURRENT_TERMINAL) {
							case 2:
								break;
							default:
								goto ZL5;
							}
							ADVANCE_LEXER;
						}
						goto ZL4;
					ZL5:;
						{
							/* BEGINNING OF ACTION: err-expected-map */
							{
#line 177 "parser.act"

		err_expected("'->'");
	
#line 315 "parser.c"
							}
							/* END OF ACTION: err-expected-map */
						}
					ZL4:;
					}
					/* END OF INLINE: 46 */
					switch (CURRENT_TERMINAL) {
					case 6:
						/* BEGINNING OF EXTRACT: token_name */
						{
#line 87 "parser.act"

		ZIt = xstrdup(lex_tokbuf(lex_state));
		if (ZIt == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 334 "parser.c"
						}
						/* END OF EXTRACT: token_name */
						break;
					default:
						goto ZL3;
					}
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: make-mapping */
					{
#line 147 "parser.act"

		assert((ZIz) != NULL);
		assert((ZIr) != NULL);

		/* TODO: "to" zone, if present */

		if (!ast_addmapping((ZIz), (ZIr), (ZIt), NULL)) {
			perror("ast_addmapping");
			goto ZL3;
		}
	
#line 356 "parser.c"
					}
					/* END OF ACTION: make-mapping */
				}
				break;
			default:
				{
					string ZIt;

					/* BEGINNING OF ACTION: no-token */
					{
#line 159 "parser.act"

		(ZIt) = NULL;
	
#line 371 "parser.c"
					}
					/* END OF ACTION: no-token */
					/* BEGINNING OF ACTION: make-mapping */
					{
#line 147 "parser.act"

		assert((ZIz) != NULL);
		assert((ZIr) != NULL);

		/* TODO: "to" zone, if present */

		if (!ast_addmapping((ZIz), (ZIr), (ZIt), NULL)) {
			perror("ast_addmapping");
			goto ZL3;
		}
	
#line 388 "parser.c"
					}
					/* END OF ACTION: make-mapping */
				}
				break;
			case 11:
				RESTORE_LEXER;
				goto ZL3;
			}
			goto ZL2;
		ZL3:;
			{
				/* BEGINNING OF ACTION: err-expected-name */
				{
#line 201 "parser.act"

		err_expected("token name");
	
#line 406 "parser.c"
				}
				/* END OF ACTION: err-expected-name */
			}
		ZL2:;
		}
		/* END OF INLINE: 44 */
	}
}

static void
p_token_Hmapping_C_Cconcaternated_Hpatterns(lex_state lex_state, act_state act_state, re *ZO70)
{
	re ZI70;

	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		re ZIr;

		p_token_Hmapping_C_Cpattern (lex_state, act_state, &ZIr);
		p_71 (lex_state, act_state, ZIr, &ZI70);
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
	*ZO70 = ZI70;
}

static void
p_71(lex_state lex_state, act_state act_state, re ZI69, re *ZO70)
{
	re ZI70;

ZL2_71:;
	switch (CURRENT_TERMINAL) {
	case 0: case 1:
		{
			re ZIb;
			re ZIr;

			p_token_Hmapping_C_Cpattern (lex_state, act_state, &ZIb);
			if ((CURRENT_TERMINAL) == 11) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: concat-regex */
			{
#line 117 "parser.act"

		assert((ZI69) != NULL);
		assert((ZIb) != NULL);

		if (!re_concat((ZI69), (ZIb))) {
			perror("re_concat");
			goto ZL1;
		}

		(ZIr) = (ZI69);
	
#line 473 "parser.c"
			}
			/* END OF ACTION: concat-regex */
			/* BEGINNING OF INLINE: 71 */
			ZI69 = ZIr;
			goto ZL2_71;
			/* END OF INLINE: 71 */
		}
		/*UNREACHED*/
	default:
		{
			ZI70 = ZI69;
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
	*ZO70 = ZI70;
}

static void
p_73(lex_state lex_state, act_state act_state, ast *ZIa)
{
	switch (CURRENT_TERMINAL) {
	case 3:
		{
			zone ZIchild;

			/* BEGINNING OF INLINE: 53 */
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
#line 185 "parser.act"

		err_expected("'..'");
	
#line 526 "parser.c"
					}
					/* END OF ACTION: err-expected-to */
				}
			ZL2:;
			}
			/* END OF INLINE: 53 */
			/* BEGINNING OF ACTION: make-zone */
			{
#line 137 "parser.act"

		assert((*ZIa) != NULL);

		(ZIchild) = ast_addzone((*ZIa));
		if ((ZIchild) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 545 "parser.c"
			}
			/* END OF ACTION: make-zone */
			p_token_Hmapping (lex_state, act_state, ZIchild);
			/* BEGINNING OF INLINE: 55 */
			{
				switch (CURRENT_TERMINAL) {
				case 4:
					{
						p_56 (lex_state, act_state);
						if ((CURRENT_TERMINAL) == 11) {
							RESTORE_LEXER;
							goto ZL5;
						}
					}
					break;
				case 7:
					{
						/* BEGINNING OF INLINE: 57 */
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
#line 189 "parser.act"

		err_expected("'{'");
	
#line 583 "parser.c"
								}
								/* END OF ACTION: err-expected-open */
							}
						ZL6:;
						}
						/* END OF INLINE: 57 */
						p_list_Hof_Hthings (lex_state, act_state, *ZIa, ZIchild);
						/* BEGINNING OF INLINE: 58 */
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
#line 193 "parser.act"

		err_expected("'}'");
	
#line 615 "parser.c"
								}
								/* END OF ACTION: err-expected-close */
							}
						ZL8:;
						}
						/* END OF INLINE: 58 */
					}
					break;
				case 11:
					RESTORE_LEXER;
					goto ZL5;
				default:
					goto ZL5;
				}
				goto ZL4;
			ZL5:;
				{
					/* BEGINNING OF ACTION: err-expected-list */
					{
#line 205 "parser.act"

		err_expected("list of token mappings or zones");
	
#line 639 "parser.c"
					}
					/* END OF ACTION: err-expected-list */
				}
			ZL4:;
			}
			/* END OF INLINE: 55 */
		}
		break;
	case 4:
		{
			p_56 (lex_state, act_state);
			if ((CURRENT_TERMINAL) == 11) {
				RESTORE_LEXER;
				goto ZL1;
			}
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
#line 129 "parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL1;
		}
	
#line 689 "parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: make-zone */
		{
#line 137 "parser.act"

		assert((ZIa) != NULL);

		(ZIz) = ast_addzone((ZIa));
		if ((ZIz) == NULL) {
			perror("ast_addzone");
			goto ZL1;
		}
	
#line 704 "parser.c"
		}
		/* END OF ACTION: make-zone */
		p_list_Hof_Hthings (lex_state, act_state, ZIa, ZIz);
		/* BEGINNING OF INLINE: 62 */
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
#line 197 "parser.act"

		err_expected("EOF");
	
#line 732 "parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL2:;
		}
		/* END OF INLINE: 62 */
	}
	goto ZL0;
ZL1:;
	{
		/* BEGINNING OF ACTION: make-ast */
		{
#line 129 "parser.act"

		(ZIa) = ast_new();
		if ((ZIa) == NULL) {
			perror("ast_new");
			goto ZL4;
		}
	
#line 753 "parser.c"
		}
		/* END OF ACTION: make-ast */
		/* BEGINNING OF ACTION: err-syntax */
		{
#line 169 "parser.act"

		fprintf(stderr, "syntax error\n");
		exit(EXIT_FAILURE);
	
#line 763 "parser.c"
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

#line 236 "parser.act"


	struct lx_ast *lx_parse(FILE *f) {
		struct act_state act_state_s;
		struct act_state *act_state;
		struct lex_state *lex_state;
		struct lx_ast *ast;

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

#line 807 "parser.c"

/* END OF FILE */
