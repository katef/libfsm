/*
 * Automatically generated from the files:
 *	cli/parser.sid
 * and
 *	cli/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 57 "cli/parser.act"


	#include <assert.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>

	#include <fsm/fsm.h>

	#include "../libfsm/xalloc.h"

	#include "lexer.h"
	#include "parser.h"

	typedef unsigned int       usint;
	typedef char *             string;
	typedef struct fsm_state * state;

	struct act_state {
		int lex_tok;
		int lex_tok_save;
	};

	#define CURRENT_TERMINAL (act_state->lex_tok)
	#define ERROR_TERMINAL   lex_unknown
	#define ADVANCE_LEXER    do { act_state->lex_tok = lex_nexttoken(lex_state); } while (0)
	#define SAVE_LEXER(tok)  do { act_state->lex_tok_save = act_state->lex_tok;  \
	                              act_state->lex_tok = tok;                      } while (0)
	#define RESTORE_LEXER    do { act_state->lex_tok = act_state->lex_tok_save;  } while (0)

	static void err_expected(const char *token) {
		fprintf(stderr, "syntax error: expected %s\n", token);
		exit(EXIT_FAILURE);
	}

#line 49 "cli/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_things(fsm, lex_state, act_state);
static void p_things_C_Cthing(fsm, lex_state, act_state);
static void p_xstart(fsm, lex_state, act_state, usint *);
static void p_50(fsm, lex_state, act_state);
static void p_xend(fsm, lex_state, act_state);
static void p_60(fsm, lex_state, act_state, usint *);
static void p_ids(fsm, lex_state, act_state);
extern void p_fsm(fsm, lex_state, act_state);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_things(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_things:;
	{
		p_things_C_Cthing (fsm, lex_state, act_state);
		/* BEGINNING OF INLINE: 58 */
		{
			switch (CURRENT_TERMINAL) {
			case (lex_id):
				{
					/* BEGINNING OF INLINE: things */
					goto ZL2_things;
					/* END OF INLINE: things */
				}
				/*UNREACHED*/
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			default:
				break;
			}
		}
		/* END OF INLINE: 58 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_things_C_Cthing(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		usint ZI59;

		switch (CURRENT_TERMINAL) {
		case (lex_id):
			/* BEGINNING OF EXTRACT: id */
			{
#line 76 "cli/parser.act"

		ZI59 = atoi(lex_tokbuf(lex_state));
	
#line 123 "cli/parser.c"
			}
			/* END OF EXTRACT: id */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		p_60 (fsm, lex_state, act_state, &ZI59);
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
p_xstart(fsm fsm, lex_state lex_state, act_state act_state, usint *ZOn)
{
	usint ZIn;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 38 */
		{
			{
				switch (CURRENT_TERMINAL) {
				case (lex_start):
					break;
				default:
					goto ZL3;
				}
				ADVANCE_LEXER;
			}
			goto ZL2;
		ZL3:;
			{
				/* BEGINNING OF ACTION: err-expected-start */
				{
#line 204 "cli/parser.act"

		err_expected("'start:'");
	
#line 172 "cli/parser.c"
				}
				/* END OF ACTION: err-expected-start */
			}
		ZL2:;
		}
		/* END OF INLINE: 38 */
		switch (CURRENT_TERMINAL) {
		case (lex_id):
			/* BEGINNING OF EXTRACT: id */
			{
#line 76 "cli/parser.act"

		ZIn = atoi(lex_tokbuf(lex_state));
	
#line 187 "cli/parser.c"
			}
			/* END OF EXTRACT: id */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		p_50 (fsm, lex_state, act_state);
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
	*ZOn = ZIn;
}

static void
p_50(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case (lex_sep):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-sep */
		{
#line 188 "cli/parser.act"

		err_expected("';'");
	
#line 233 "cli/parser.c"
		}
		/* END OF ACTION: err-expected-sep */
	}
}

static void
p_xend(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 35 */
		{
			{
				switch (CURRENT_TERMINAL) {
				case (lex_end):
					break;
				default:
					goto ZL3;
				}
				ADVANCE_LEXER;
			}
			goto ZL2;
		ZL3:;
			{
				/* BEGINNING OF ACTION: err-expected-end */
				{
#line 208 "cli/parser.act"

		err_expected("'end:'");
	
#line 266 "cli/parser.c"
				}
				/* END OF ACTION: err-expected-end */
			}
		ZL2:;
		}
		/* END OF INLINE: 35 */
		p_ids (fsm, lex_state, act_state);
		p_50 (fsm, lex_state, act_state);
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
p_60(fsm fsm, lex_state lex_state, act_state act_state, usint *ZI59)
{
	switch (CURRENT_TERMINAL) {
	case (lex_arrow):
		{
			usint ZIb;
			state ZIx;
			state ZIy;

			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (lex_id):
				/* BEGINNING OF EXTRACT: id */
				{
#line 76 "cli/parser.act"

		ZIb = atoi(lex_tokbuf(lex_state));
	
#line 305 "cli/parser.c"
				}
				/* END OF EXTRACT: id */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: add-state */
			{
#line 98 "cli/parser.act"

		if ((*ZI59) == 0) {
			fprintf(stderr, "special id 0 is reserved\n");
			goto ZL1;
		}

		(ZIx) = fsm_addstate(fsm, (*ZI59));

		if ((ZIx) == NULL) {
			perror("fsm_addstate");
			exit(EXIT_FAILURE);
		}
	
#line 329 "cli/parser.c"
			}
			/* END OF ACTION: add-state */
			/* BEGINNING OF ACTION: add-state */
			{
#line 98 "cli/parser.act"

		if ((ZIb) == 0) {
			fprintf(stderr, "special id 0 is reserved\n");
			goto ZL1;
		}

		(ZIy) = fsm_addstate(fsm, (ZIb));

		if ((ZIy) == NULL) {
			perror("fsm_addstate");
			exit(EXIT_FAILURE);
		}
	
#line 348 "cli/parser.c"
			}
			/* END OF ACTION: add-state */
			/* BEGINNING OF INLINE: 45 */
			{
				switch (CURRENT_TERMINAL) {
				case (lex_any):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: add-edge-any */
						{
#line 170 "cli/parser.act"

		if (!fsm_addedge_any(fsm, (ZIx), (ZIy))) {
			perror("fsm_addedge_any");
			exit(EXIT_FAILURE);
		}
	
#line 366 "cli/parser.c"
						}
						/* END OF ACTION: add-edge-any */
					}
					break;
				case (lex_literal):
					{
						string ZIs;

						/* BEGINNING OF EXTRACT: literal */
						{
#line 88 "cli/parser.act"

		ZIs = xstrdup(lex_tokbuf(lex_state));
		if (ZIs == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 385 "cli/parser.c"
						}
						/* END OF EXTRACT: literal */
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: add-edge-literal */
						{
#line 148 "cli/parser.act"

		assert((ZIs) != NULL);

		/* TODO: convert to single char */
		if (strlen((ZIs)) != 1) {
			fprintf(stderr, "literals must have exactly one character\n");
			goto ZL3;
		}

		if (!fsm_addedge_literal(fsm, (ZIx), (ZIy), (ZIs)[0])) {
			perror("fsm_addedge_literal");
			exit(EXIT_FAILURE);
		}
	
#line 406 "cli/parser.c"
						}
						/* END OF ACTION: add-edge-literal */
						/* BEGINNING OF ACTION: free */
						{
#line 177 "cli/parser.act"

		free((ZIs));
	
#line 415 "cli/parser.c"
						}
						/* END OF ACTION: free */
					}
					break;
				default:
					{
						/* BEGINNING OF ACTION: add-edge-epsilon */
						{
#line 163 "cli/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIy))) {
			perror("fsm_addedge_epsilon");
			exit(EXIT_FAILURE);
		}
	
#line 431 "cli/parser.c"
						}
						/* END OF ACTION: add-edge-epsilon */
					}
					break;
				}
				goto ZL2;
			ZL3:;
				{
					/* BEGINNING OF ACTION: err-expected-trans */
					{
#line 192 "cli/parser.act"

		err_expected("transition");
	
#line 446 "cli/parser.c"
					}
					/* END OF ACTION: err-expected-trans */
				}
			ZL2:;
			}
			/* END OF INLINE: 45 */
			p_50 (fsm, lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (lex_equals):
		{
			string ZIs;

			/* BEGINNING OF INLINE: 49 */
			{
				{
					switch (CURRENT_TERMINAL) {
					case (lex_equals):
						break;
					default:
						goto ZL5;
					}
					ADVANCE_LEXER;
				}
				goto ZL4;
			ZL5:;
				{
					/* BEGINNING OF ACTION: err-expected-equals */
					{
#line 200 "cli/parser.act"

		err_expected("'='");
	
#line 484 "cli/parser.c"
					}
					/* END OF ACTION: err-expected-equals */
				}
			ZL4:;
			}
			/* END OF INLINE: 49 */
			switch (CURRENT_TERMINAL) {
			case (lex_label):
				/* BEGINNING OF EXTRACT: label */
				{
#line 80 "cli/parser.act"

		ZIs = xstrdup(lex_tokbuf(lex_state));
		if (ZIs == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 503 "cli/parser.c"
				}
				/* END OF EXTRACT: label */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			p_50 (fsm, lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: mark-colour */
			{
#line 138 "cli/parser.act"

		struct fsm_state *state;

		state = fsm_getstatebyid(fsm, (*ZI59));
		if (state == NULL) {
			fprintf(stderr, "unrecognised state: %u\n", (*ZI59));
			goto ZL1;
		}

		fsm_setopaque(fsm, state, (ZIs));
	
#line 530 "cli/parser.c"
			}
			/* END OF ACTION: mark-colour */
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
p_ids(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_ids:;
	{
		usint ZIn;

		switch (CURRENT_TERMINAL) {
		case (lex_id):
			/* BEGINNING OF EXTRACT: id */
			{
#line 76 "cli/parser.act"

		ZIn = atoi(lex_tokbuf(lex_state));
	
#line 564 "cli/parser.c"
			}
			/* END OF EXTRACT: id */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: mark-end */
		{
#line 126 "cli/parser.act"

		struct fsm_state *state;

		state = fsm_getstatebyid(fsm, (ZIn));
		if (state == NULL) {
			fprintf(stderr, "unrecognised end state: %u\n", (ZIn));
			goto ZL1;
		}

		fsm_setend(fsm, state, 1);
	
#line 586 "cli/parser.c"
		}
		/* END OF ACTION: mark-end */
		/* BEGINNING OF INLINE: 57 */
		{
			switch (CURRENT_TERMINAL) {
			case (lex_comma):
				{
					/* BEGINNING OF INLINE: 33 */
					{
						{
							switch (CURRENT_TERMINAL) {
							case (lex_comma):
								break;
							default:
								goto ZL5;
							}
							ADVANCE_LEXER;
						}
						goto ZL4;
					ZL5:;
						{
							/* BEGINNING OF ACTION: err-expected-comma */
							{
#line 196 "cli/parser.act"

		err_expected("','");
	
#line 614 "cli/parser.c"
							}
							/* END OF ACTION: err-expected-comma */
						}
					ZL4:;
					}
					/* END OF INLINE: 33 */
					/* BEGINNING OF INLINE: ids */
					goto ZL2_ids;
					/* END OF INLINE: ids */
				}
				/*UNREACHED*/
			default:
				break;
			}
		}
		/* END OF INLINE: 57 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

void
p_fsm(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		usint ZIn;

		p_things (fsm, lex_state, act_state);
		p_xstart (fsm, lex_state, act_state, &ZIn);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
		/* BEGINNING OF ACTION: mark-start */
		{
#line 114 "cli/parser.act"

		struct fsm_state *state;

		state = fsm_getstatebyid(fsm, (ZIn));
		if (state == NULL) {
			fprintf(stderr, "unrecognised start state: %u\n", (ZIn));
			goto ZL1;
		}

		fsm_setstart(fsm, state);
	
#line 667 "cli/parser.c"
		}
		/* END OF ACTION: mark-start */
		/* BEGINNING OF INLINE: 55 */
		{
			switch (CURRENT_TERMINAL) {
			case (lex_end):
				{
					p_xend (fsm, lex_state, act_state);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				break;
			}
		}
		/* END OF INLINE: 55 */
		switch (CURRENT_TERMINAL) {
		case (lex_eof):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-parse */
		{
#line 184 "cli/parser.act"

		fprintf(stderr, "parse error\n");
		exit(EXIT_FAILURE);
	
#line 705 "cli/parser.c"
		}
		/* END OF ACTION: err-parse */
	}
}

/* BEGINNING OF TRAILER */

#line 239 "cli/parser.act"


	struct fsm *fsm_parse(struct fsm *fsm, FILE *f) {
		struct act_state act_state_s;
		struct act_state *act_state;
		struct lex_state *lex_state;

		assert(fsm != NULL);
		assert(f != NULL);

		/* TODO: perhaps create the fsm struct here, rather than passing one in */

		lex_state = lex_init(f);
		if (lex_state == NULL) {
			perror("lex_init");
			return NULL;
		}

		/* This is a workaround for ADVANCE_LEXER assuming a pointer */
		act_state = &act_state_s;

		ADVANCE_LEXER;
		p_fsm(fsm, lex_state, act_state);

		lex_free(lex_state);

		return fsm;
	}

#line 743 "cli/parser.c"

/* END OF FILE */
