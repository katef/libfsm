/*
 * Automatically generated from the files:
 *	parser.sid
 * and
 *	parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 51 "parser.act"


	#include <assert.h>
	#include <stdio.h>
	#include <stdlib.h>

	#include <re/re.h>

	#include "lexer.h"
	#include "parser.h"
	#include "xalloc.h"
	#include "ast.h"

	typedef char *      string;
	typedef struct re * re;

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

#line 47 "parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Hthings(lex_state, act_state);
static void p_list_Hof_Hthings_C_Cthing(lex_state, act_state);
static void p_47(lex_state, act_state);
static void p_56(lex_state, act_state, re, re *);
static void p_58(lex_state, act_state);
static void p_token_Hmapping_C_Cpattern(lex_state, act_state, re *);
static void p_token_Hmapping(lex_state, act_state);
static void p_token_Hmapping_C_Cconcaternated_Hpatterns(lex_state, act_state, re *);
extern void p_lx(lex_state, act_state);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Hthings(lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_list_Hof_Hthings:;
	{
		p_list_Hof_Hthings_C_Cthing (lex_state, act_state);
		/* BEGINNING OF INLINE: 53 */
		{
			switch (CURRENT_TERMINAL) {
			case (lex_pattern__regex): case (lex_pattern__literal):
				{
					/* BEGINNING OF INLINE: list-of-things */
					goto ZL2_list_Hof_Hthings;
					/* END OF INLINE: list-of-things */
				}
				/*UNREACHED*/
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			default:
				break;
			}
		}
		/* END OF INLINE: 53 */
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-list */
		{
#line 161 "parser.act"

		err_expected("list of token mappings or zones");
	
#line 108 "parser.c"
		}
		/* END OF ACTION: err-expected-list */
	}
}

static void
p_list_Hof_Hthings_C_Cthing(lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_token_Hmapping (lex_state, act_state);
		p_58 (lex_state, act_state);
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
p_47(lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case (lex_semi):
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
#line 140 "parser.act"

		err_expected("';'");
	
#line 158 "parser.c"
		}
		/* END OF ACTION: err-expected-semi */
	}
}

static void
p_56(lex_state lex_state, act_state act_state, re ZI54, re *ZO55)
{
	re ZI55;

ZL2_56:;
	switch (CURRENT_TERMINAL) {
	case (lex_pattern__regex): case (lex_pattern__literal):
		{
			re ZIb;
			re ZIr;

			p_token_Hmapping_C_Cpattern (lex_state, act_state, &ZIb);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: concat-regex */
			{
#line 111 "parser.act"

		assert((ZI54) != NULL);
		assert((ZIb) != NULL);

		(ZIr) = re_concat((ZI54), (ZIb));
		if ((ZIr) == NULL) {
			perror("re_concat");
			goto ZL1;
		}
	
#line 194 "parser.c"
			}
			/* END OF ACTION: concat-regex */
			/* BEGINNING OF INLINE: 56 */
			ZI54 = ZIr;
			goto ZL2_56;
			/* END OF INLINE: 56 */
		}
		/*UNREACHED*/
	default:
		{
			ZI55 = ZI54;
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
	*ZO55 = ZI55;
}

static void
p_58(lex_state lex_state, act_state act_state)
{
	switch (CURRENT_TERMINAL) {
	case (lex_to):
		{
			/* BEGINNING OF INLINE: 45 */
			{
				{
					switch (CURRENT_TERMINAL) {
					case (lex_to):
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
#line 144 "parser.act"

		err_expected("'..'");
	
#line 245 "parser.c"
					}
					/* END OF ACTION: err-expected-to */
				}
			ZL2:;
			}
			/* END OF INLINE: 45 */
			p_token_Hmapping (lex_state, act_state);
			/* BEGINNING OF INLINE: 46 */
			{
				switch (CURRENT_TERMINAL) {
				case (lex_semi):
					{
						p_47 (lex_state, act_state);
						if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
							RESTORE_LEXER;
							goto ZL5;
						}
					}
					break;
				case (lex_open):
					{
						/* BEGINNING OF INLINE: 48 */
						{
							{
								switch (CURRENT_TERMINAL) {
								case (lex_open):
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
#line 148 "parser.act"

		err_expected("'{'");
	
#line 287 "parser.c"
								}
								/* END OF ACTION: err-expected-open */
							}
						ZL6:;
						}
						/* END OF INLINE: 48 */
						p_list_Hof_Hthings (lex_state, act_state);
						/* BEGINNING OF INLINE: 49 */
						{
							if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
								RESTORE_LEXER;
								goto ZL5;
							}
							{
								switch (CURRENT_TERMINAL) {
								case (lex_close):
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
#line 152 "parser.act"

		err_expected("'}'");
	
#line 319 "parser.c"
								}
								/* END OF ACTION: err-expected-close */
							}
						ZL8:;
						}
						/* END OF INLINE: 49 */
					}
					break;
				case (ERROR_TERMINAL):
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
#line 161 "parser.act"

		err_expected("list of token mappings or zones");
	
#line 343 "parser.c"
					}
					/* END OF ACTION: err-expected-list */
				}
			ZL4:;
			}
			/* END OF INLINE: 46 */
		}
		break;
	case (lex_semi):
		{
			p_47 (lex_state, act_state);
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
p_token_Hmapping_C_Cpattern(lex_state lex_state, act_state act_state, re *ZOr)
{
	re ZIr;

	switch (CURRENT_TERMINAL) {
	case (lex_pattern__literal):
		{
			string ZIs;

			/* BEGINNING OF EXTRACT: pattern_literal */
			{
#line 73 "parser.act"

		ZIs = xstrdup(lex_tokbuf(lex_state));
		if (ZIs == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 392 "parser.c"
			}
			/* END OF EXTRACT: pattern_literal */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: compile-literal */
			{
#line 91 "parser.act"

		assert((ZIs) != NULL);

		(ZIr) = re_new_comp(RE_LITERAL, re_getc_str, &(ZIs), 0, NULL, NULL);
		if ((ZIr) == NULL) {
			/* TODO: use *err */
			goto ZL1;
		}
	
#line 408 "parser.c"
			}
			/* END OF ACTION: compile-literal */
		}
		break;
	case (lex_pattern__regex):
		{
			string ZIs;

			/* BEGINNING OF EXTRACT: pattern_regex */
			{
#line 65 "parser.act"

		ZIs = xstrdup(lex_tokbuf(lex_state));
		if (ZIs == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 427 "parser.c"
			}
			/* END OF EXTRACT: pattern_regex */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: compile-regex */
			{
#line 101 "parser.act"

		assert((ZIs) != NULL);

		(ZIr) = re_new_comp(RE_SIMPLE, re_getc_str, &(ZIs), 0, NULL, NULL);
		if ((ZIr) == NULL) {
			/* TODO: use *err */
			goto ZL1;
		}
	
#line 443 "parser.c"
			}
			/* END OF ACTION: compile-regex */
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
p_token_Hmapping(lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		re ZIr;

		p_token_Hmapping_C_Cconcaternated_Hpatterns (lex_state, act_state, &ZIr);
		/* BEGINNING OF INLINE: 36 */
		{
			switch (CURRENT_TERMINAL) {
			case (lex_map):
				{
					string ZI38;

					/* BEGINNING OF INLINE: 37 */
					{
						{
							switch (CURRENT_TERMINAL) {
							case (lex_map):
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
#line 136 "parser.act"

		err_expected("'->'");
	
#line 498 "parser.c"
							}
							/* END OF ACTION: err-expected-map */
						}
					ZL4:;
					}
					/* END OF INLINE: 37 */
					switch (CURRENT_TERMINAL) {
					case (lex_token__name):
						/* BEGINNING OF EXTRACT: token_name */
						{
#line 81 "parser.act"

		ZI38 = xstrdup(lex_tokbuf(lex_state));
		if (ZI38 == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 517 "parser.c"
						}
						/* END OF EXTRACT: token_name */
						break;
					default:
						goto ZL3;
					}
					ADVANCE_LEXER;
				}
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL3;
			default:
				break;
			}
			goto ZL2;
		ZL3:;
			{
				/* BEGINNING OF ACTION: err-expected-name */
				{
#line 157 "parser.act"

		err_expected("token name");
	
#line 542 "parser.c"
				}
				/* END OF ACTION: err-expected-name */
			}
		ZL2:;
		}
		/* END OF INLINE: 36 */
	}
}

static void
p_token_Hmapping_C_Cconcaternated_Hpatterns(lex_state lex_state, act_state act_state, re *ZO55)
{
	re ZI55;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		re ZIr;

		p_token_Hmapping_C_Cpattern (lex_state, act_state, &ZIr);
		p_56 (lex_state, act_state, ZIr, &ZI55);
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
	*ZO55 = ZI55;
}

void
p_lx(lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_list_Hof_Hthings (lex_state, act_state);
		switch (CURRENT_TERMINAL) {
		case (lex_eof):
			break;
		case (ERROR_TERMINAL):
			RESTORE_LEXER;
			goto ZL1;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-syntax */
		{
#line 128 "parser.act"

		fprintf(stderr, "syntax error\n");
		exit(EXIT_FAILURE);
	
#line 607 "parser.c"
		}
		/* END OF ACTION: err-syntax */
	}
}

/* BEGINNING OF TRAILER */

#line 187 "parser.act"


	void lx_parse(FILE *f) {
		struct act_state act_state_s;
		struct act_state *act_state;
		struct lex_state *lex_state;

		assert(f != NULL);

		lex_state = lex_init(f);
		if (lex_state == NULL) {
			perror("lex_init");
			return;
		}

		/* This is a workaround for ADVANCE_LEXER assuming a pointer */
		act_state = &act_state_s;

		ADVANCE_LEXER;	/* XXX: what if the first token is unrecognised? */
		p_lx(lex_state, act_state);

		lex_free(lex_state);
	}

#line 640 "parser.c"

/* END OF FILE */
