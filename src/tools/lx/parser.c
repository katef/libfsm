/*
 * Automatically generated from the files:
 *	parser.sid
 * and
 *	parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 41 "parser.act"


	#include <assert.h>
	#include <stdio.h>
	#include <stdlib.h>

	#include "lexer.h"
	#include "parser.h"

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

#line 40 "parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Hthings(lex_state, act_state);
static void p_token_Hmapping_C_Calternative_Hpatterns(lex_state, act_state);
static void p_list_Hof_Hthings_C_Cthing(lex_state, act_state);
static void p_39(lex_state, act_state);
static void p_48(lex_state, act_state);
static void p_token_Hmapping(lex_state, act_state);
static void p_token_Hmapping_C_Cconcaternated_Hpatterns(lex_state, act_state);
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
		/* BEGINNING OF INLINE: 45 */
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
		/* END OF INLINE: 45 */
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-list */
		{
#line 92 "parser.act"

		err_expected("list of token mappings or zones");
	
#line 100 "parser.c"
		}
		/* END OF ACTION: err-expected-list */
	}
}

static void
p_token_Hmapping_C_Calternative_Hpatterns(lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_token_Hmapping_C_Calternative_Hpatterns:;
	{
		p_token_Hmapping_C_Cconcaternated_Hpatterns (lex_state, act_state);
		/* BEGINNING OF INLINE: 46 */
		{
			switch (CURRENT_TERMINAL) {
			case (lex_alt):
				{
					/* BEGINNING OF INLINE: 28 */
					{
						{
							switch (CURRENT_TERMINAL) {
							case (lex_alt):
								break;
							default:
								goto ZL5;
							}
							ADVANCE_LEXER;
						}
						goto ZL4;
					ZL5:;
						{
							/* BEGINNING OF ACTION: err-expected-alt */
							{
#line 63 "parser.act"

		err_expected("'|'");
	
#line 140 "parser.c"
							}
							/* END OF ACTION: err-expected-alt */
						}
					ZL4:;
					}
					/* END OF INLINE: 28 */
					/* BEGINNING OF INLINE: token-mapping::alternative-patterns */
					goto ZL2_token_Hmapping_C_Calternative_Hpatterns;
					/* END OF INLINE: token-mapping::alternative-patterns */
				}
				/*UNREACHED*/
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			default:
				break;
			}
		}
		/* END OF INLINE: 46 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Hthings_C_Cthing(lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_token_Hmapping (lex_state, act_state);
		p_48 (lex_state, act_state);
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
p_39(lex_state lex_state, act_state act_state)
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
#line 71 "parser.act"

		err_expected("';'");
	
#line 211 "parser.c"
		}
		/* END OF ACTION: err-expected-semi */
	}
}

static void
p_48(lex_state lex_state, act_state act_state)
{
	switch (CURRENT_TERMINAL) {
	case (lex_to):
		{
			/* BEGINNING OF INLINE: 37 */
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
#line 75 "parser.act"

		err_expected("'..'");
	
#line 243 "parser.c"
					}
					/* END OF ACTION: err-expected-to */
				}
			ZL2:;
			}
			/* END OF INLINE: 37 */
			p_token_Hmapping (lex_state, act_state);
			/* BEGINNING OF INLINE: 38 */
			{
				switch (CURRENT_TERMINAL) {
				case (lex_semi):
					{
						p_39 (lex_state, act_state);
						if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
							RESTORE_LEXER;
							goto ZL5;
						}
					}
					break;
				case (lex_open):
					{
						/* BEGINNING OF INLINE: 40 */
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
#line 79 "parser.act"

		err_expected("'{'");
	
#line 285 "parser.c"
								}
								/* END OF ACTION: err-expected-open */
							}
						ZL6:;
						}
						/* END OF INLINE: 40 */
						p_list_Hof_Hthings (lex_state, act_state);
						/* BEGINNING OF INLINE: 41 */
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
#line 83 "parser.act"

		err_expected("'}'");
	
#line 317 "parser.c"
								}
								/* END OF ACTION: err-expected-close */
							}
						ZL8:;
						}
						/* END OF INLINE: 41 */
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
#line 92 "parser.act"

		err_expected("list of token mappings or zones");
	
#line 341 "parser.c"
					}
					/* END OF ACTION: err-expected-list */
				}
			ZL4:;
			}
			/* END OF INLINE: 38 */
		}
		break;
	case (lex_semi):
		{
			p_39 (lex_state, act_state);
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
p_token_Hmapping(lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_token_Hmapping_C_Calternative_Hpatterns (lex_state, act_state);
		/* BEGINNING OF INLINE: 29 */
		{
			switch (CURRENT_TERMINAL) {
			case (lex_map):
				{
					/* BEGINNING OF INLINE: 30 */
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
#line 67 "parser.act"

		err_expected("'->'");
	
#line 403 "parser.c"
							}
							/* END OF ACTION: err-expected-map */
						}
					ZL4:;
					}
					/* END OF INLINE: 30 */
					switch (CURRENT_TERMINAL) {
					case (lex_token__name):
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
#line 88 "parser.act"

		err_expected("token name");
	
#line 434 "parser.c"
				}
				/* END OF ACTION: err-expected-name */
			}
		ZL2:;
		}
		/* END OF INLINE: 29 */
	}
}

static void
p_token_Hmapping_C_Cconcaternated_Hpatterns(lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_token_Hmapping_C_Cconcaternated_Hpatterns:;
	{
		/* BEGINNING OF INLINE: token-mapping::pattern */
		{
			switch (CURRENT_TERMINAL) {
			case (lex_pattern__literal):
				{
					ADVANCE_LEXER;
				}
				break;
			case (lex_pattern__regex):
				{
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: token-mapping::pattern */
		/* BEGINNING OF INLINE: 47 */
		{
			switch (CURRENT_TERMINAL) {
			case (lex_pattern__regex): case (lex_pattern__literal):
				{
					/* BEGINNING OF INLINE: token-mapping::concaternated-patterns */
					goto ZL2_token_Hmapping_C_Cconcaternated_Hpatterns;
					/* END OF INLINE: token-mapping::concaternated-patterns */
				}
				/*UNREACHED*/
			default:
				break;
			}
		}
		/* END OF INLINE: 47 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
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
#line 59 "parser.act"

		fprintf(stderr, "syntax error\n");
		exit(EXIT_FAILURE);
	
#line 521 "parser.c"
		}
		/* END OF ACTION: err-syntax */
	}
}

/* BEGINNING OF TRAILER */

#line 118 "parser.act"


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

#line 554 "parser.c"

/* END OF FILE */
