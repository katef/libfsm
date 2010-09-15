/*
 * Automatically generated from the files:
 *	cli/parser.sid
 * and
 *	cli/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 62 "cli/parser.act"


	#include <assert.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>

	#include <fsm/fsm.h>

	#include "../libfsm/xalloc.h"

	#include "lexer.h"
	#include "parser.h"

	typedef char *             string;
	typedef struct fsm_state * state;

	struct act_state {
		int lex_tok;
		int lex_tok_save;
		struct act_statelist *sl;
	};

	struct act_statelist {
		char *id;
		struct fsm_state *state;
		struct act_statelist *next;
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

#line 55 "cli/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_things(fsm, lex_state, act_state);
static void p_things_C_Cthing(fsm, lex_state, act_state);
static void p_xstart(fsm, lex_state, act_state, string *);
static void p_xend(fsm, lex_state, act_state);
static void p_51(fsm, lex_state, act_state);
static void p_61(fsm, lex_state, act_state, string *);
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
		/* BEGINNING OF INLINE: 59 */
		{
			switch (CURRENT_TERMINAL) {
			case (lex_ident): case (lex_label):
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
		/* END OF INLINE: 59 */
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
		string ZI60;

		/* BEGINNING OF INLINE: id */
		{
			switch (CURRENT_TERMINAL) {
			case (lex_ident):
				{
					/* BEGINNING OF EXTRACT: ident */
					{
#line 81 "cli/parser.act"

		ZI60 = xstrdup(lex_tokbuf(lex_state));
		if (ZI60 == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 136 "cli/parser.c"
					}
					/* END OF EXTRACT: ident */
					ADVANCE_LEXER;
				}
				break;
			case (lex_label):
				{
					/* BEGINNING OF EXTRACT: label */
					{
#line 89 "cli/parser.act"

		ZI60 = xstrdup(lex_tokbuf(lex_state));
		if (ZI60 == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 154 "cli/parser.c"
					}
					/* END OF EXTRACT: label */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: id */
		p_61 (fsm, lex_state, act_state, &ZI60);
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
p_xstart(fsm fsm, lex_state lex_state, act_state act_state, string *ZOn)
{
	string ZIn;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 39 */
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
#line 229 "cli/parser.act"

		err_expected("'start:'");
	
#line 206 "cli/parser.c"
				}
				/* END OF ACTION: err-expected-start */
			}
		ZL2:;
		}
		/* END OF INLINE: 39 */
		/* BEGINNING OF INLINE: id */
		{
			switch (CURRENT_TERMINAL) {
			case (lex_ident):
				{
					/* BEGINNING OF EXTRACT: ident */
					{
#line 81 "cli/parser.act"

		ZIn = xstrdup(lex_tokbuf(lex_state));
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 228 "cli/parser.c"
					}
					/* END OF EXTRACT: ident */
					ADVANCE_LEXER;
				}
				break;
			case (lex_label):
				{
					/* BEGINNING OF EXTRACT: label */
					{
#line 89 "cli/parser.act"

		ZIn = xstrdup(lex_tokbuf(lex_state));
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 246 "cli/parser.c"
					}
					/* END OF EXTRACT: label */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: id */
		p_51 (fsm, lex_state, act_state);
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
p_xend(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 36 */
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
#line 233 "cli/parser.act"

		err_expected("'end:'");
	
#line 298 "cli/parser.c"
				}
				/* END OF ACTION: err-expected-end */
			}
		ZL2:;
		}
		/* END OF INLINE: 36 */
		p_ids (fsm, lex_state, act_state);
		p_51 (fsm, lex_state, act_state);
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
p_51(fsm fsm, lex_state lex_state, act_state act_state)
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
#line 213 "cli/parser.act"

		err_expected("';'");
	
#line 342 "cli/parser.c"
		}
		/* END OF ACTION: err-expected-sep */
	}
}

static void
p_61(fsm fsm, lex_state lex_state, act_state act_state, string *ZI60)
{
	switch (CURRENT_TERMINAL) {
	case (lex_arrow):
		{
			string ZIb;
			state ZIx;
			state ZIy;

			ADVANCE_LEXER;
			/* BEGINNING OF INLINE: id */
			{
				switch (CURRENT_TERMINAL) {
				case (lex_ident):
					{
						/* BEGINNING OF EXTRACT: ident */
						{
#line 81 "cli/parser.act"

		ZIb = xstrdup(lex_tokbuf(lex_state));
		if (ZIb == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 374 "cli/parser.c"
						}
						/* END OF EXTRACT: ident */
						ADVANCE_LEXER;
					}
					break;
				case (lex_label):
					{
						/* BEGINNING OF EXTRACT: label */
						{
#line 89 "cli/parser.act"

		ZIb = xstrdup(lex_tokbuf(lex_state));
		if (ZIb == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 392 "cli/parser.c"
						}
						/* END OF EXTRACT: label */
						ADVANCE_LEXER;
					}
					break;
				default:
					goto ZL1;
				}
			}
			/* END OF INLINE: id */
			/* BEGINNING OF ACTION: add-state */
			{
#line 101 "cli/parser.act"

		struct act_statelist *p;

		assert((*ZI60) != NULL);

		for (p = act_state->sl; p != NULL; p = p->next) {
			assert(p->id != NULL);
			assert(p->state != NULL);

			if (0 == strcmp(p->id, (*ZI60))) {
				(ZIx) = p->state;
				break;
			}
		}

		if (p == NULL) {
			struct act_statelist *new;

			new = malloc(sizeof *new);
			if (new == NULL) {
				perror("malloc");
				exit(EXIT_FAILURE);
			}

			new->id = xstrdup((*ZI60));
			if (new->id == NULL) {
				perror("xstrdup");
				exit(EXIT_FAILURE);
			}

			(ZIx) = fsm_addstate(fsm);
			if ((ZIx) == NULL) {
				perror("fsm_addstate");
				exit(EXIT_FAILURE);
			}

			new->state = (ZIx);

			new->next = act_state->sl;
			act_state->sl = new;
		}
	
#line 448 "cli/parser.c"
			}
			/* END OF ACTION: add-state */
			/* BEGINNING OF ACTION: add-state */
			{
#line 101 "cli/parser.act"

		struct act_statelist *p;

		assert((ZIb) != NULL);

		for (p = act_state->sl; p != NULL; p = p->next) {
			assert(p->id != NULL);
			assert(p->state != NULL);

			if (0 == strcmp(p->id, (ZIb))) {
				(ZIy) = p->state;
				break;
			}
		}

		if (p == NULL) {
			struct act_statelist *new;

			new = malloc(sizeof *new);
			if (new == NULL) {
				perror("malloc");
				exit(EXIT_FAILURE);
			}

			new->id = xstrdup((ZIb));
			if (new->id == NULL) {
				perror("xstrdup");
				exit(EXIT_FAILURE);
			}

			(ZIy) = fsm_addstate(fsm);
			if ((ZIy) == NULL) {
				perror("fsm_addstate");
				exit(EXIT_FAILURE);
			}

			new->state = (ZIy);

			new->next = act_state->sl;
			act_state->sl = new;
		}
	
#line 496 "cli/parser.c"
			}
			/* END OF ACTION: add-state */
			/* BEGINNING OF INLINE: 46 */
			{
				switch (CURRENT_TERMINAL) {
				case (lex_any):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: add-edge-any */
						{
#line 197 "cli/parser.act"

		if (!fsm_addedge_any(fsm, (ZIx), (ZIy))) {
			perror("fsm_addedge_any");
			exit(EXIT_FAILURE);
		}
	
#line 514 "cli/parser.c"
						}
						/* END OF ACTION: add-edge-any */
					}
					break;
				case (lex_label):
					{
						string ZIs;

						/* BEGINNING OF EXTRACT: label */
						{
#line 89 "cli/parser.act"

		ZIs = xstrdup(lex_tokbuf(lex_state));
		if (ZIs == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 533 "cli/parser.c"
						}
						/* END OF EXTRACT: label */
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: add-edge-literal */
						{
#line 182 "cli/parser.act"

		assert((ZIs) != NULL);

		/* TODO: convert to single char in the grammar? */
		if (strlen((ZIs)) != 1) {
			fprintf(stderr, "edge literals must have exactly one character\n");
			goto ZL4;
		}

		if (!fsm_addedge_literal(fsm, (ZIx), (ZIy), (ZIs)[0])) {
			perror("fsm_addedge_literal");
			exit(EXIT_FAILURE);
		}
	
#line 554 "cli/parser.c"
						}
						/* END OF ACTION: add-edge-literal */
						/* BEGINNING OF ACTION: free */
						{
#line 154 "cli/parser.act"

		free((ZIs));
	
#line 563 "cli/parser.c"
						}
						/* END OF ACTION: free */
					}
					break;
				default:
					{
						/* BEGINNING OF ACTION: add-edge-epsilon */
						{
#line 204 "cli/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIy))) {
			perror("fsm_addedge_epsilon");
			exit(EXIT_FAILURE);
		}
	
#line 579 "cli/parser.c"
						}
						/* END OF ACTION: add-edge-epsilon */
					}
					break;
				}
				goto ZL3;
			ZL4:;
				{
					/* BEGINNING OF ACTION: err-expected-trans */
					{
#line 217 "cli/parser.act"

		err_expected("transition");
	
#line 594 "cli/parser.c"
					}
					/* END OF ACTION: err-expected-trans */
				}
			ZL3:;
			}
			/* END OF INLINE: 46 */
			p_51 (fsm, lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (lex_equals):
		{
			state ZIs;
			string ZIo;

			/* BEGINNING OF ACTION: add-state */
			{
#line 101 "cli/parser.act"

		struct act_statelist *p;

		assert((*ZI60) != NULL);

		for (p = act_state->sl; p != NULL; p = p->next) {
			assert(p->id != NULL);
			assert(p->state != NULL);

			if (0 == strcmp(p->id, (*ZI60))) {
				(ZIs) = p->state;
				break;
			}
		}

		if (p == NULL) {
			struct act_statelist *new;

			new = malloc(sizeof *new);
			if (new == NULL) {
				perror("malloc");
				exit(EXIT_FAILURE);
			}

			new->id = xstrdup((*ZI60));
			if (new->id == NULL) {
				perror("xstrdup");
				exit(EXIT_FAILURE);
			}

			(ZIs) = fsm_addstate(fsm);
			if ((ZIs) == NULL) {
				perror("fsm_addstate");
				exit(EXIT_FAILURE);
			}

			new->state = (ZIs);

			new->next = act_state->sl;
			act_state->sl = new;
		}
	
#line 658 "cli/parser.c"
			}
			/* END OF ACTION: add-state */
			/* BEGINNING OF INLINE: 49 */
			{
				{
					switch (CURRENT_TERMINAL) {
					case (lex_equals):
						break;
					default:
						goto ZL6;
					}
					ADVANCE_LEXER;
				}
				goto ZL5;
			ZL6:;
				{
					/* BEGINNING OF ACTION: err-expected-equals */
					{
#line 225 "cli/parser.act"

		err_expected("'='");
	
#line 681 "cli/parser.c"
					}
					/* END OF ACTION: err-expected-equals */
				}
			ZL5:;
			}
			/* END OF INLINE: 49 */
			switch (CURRENT_TERMINAL) {
			case (lex_label):
				/* BEGINNING OF EXTRACT: label */
				{
#line 89 "cli/parser.act"

		ZIo = xstrdup(lex_tokbuf(lex_state));
		if (ZIo == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 700 "cli/parser.c"
				}
				/* END OF EXTRACT: label */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			p_51 (fsm, lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: mark-colour */
			{
#line 172 "cli/parser.act"

		assert((ZIs) != NULL);
		assert((ZIo) != NULL);

		if (!fsm_addopaque(fsm, (ZIs), (ZIo))) {
			perror("fsm_addopaque");
			goto ZL1;
		}
	
#line 725 "cli/parser.c"
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
		string ZIn;
		state ZIs;

		/* BEGINNING OF INLINE: id */
		{
			switch (CURRENT_TERMINAL) {
			case (lex_ident):
				{
					/* BEGINNING OF EXTRACT: ident */
					{
#line 81 "cli/parser.act"

		ZIn = xstrdup(lex_tokbuf(lex_state));
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 767 "cli/parser.c"
					}
					/* END OF EXTRACT: ident */
					ADVANCE_LEXER;
				}
				break;
			case (lex_label):
				{
					/* BEGINNING OF EXTRACT: label */
					{
#line 89 "cli/parser.act"

		ZIn = xstrdup(lex_tokbuf(lex_state));
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 785 "cli/parser.c"
					}
					/* END OF EXTRACT: label */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: id */
		/* BEGINNING OF ACTION: add-state */
		{
#line 101 "cli/parser.act"

		struct act_statelist *p;

		assert((ZIn) != NULL);

		for (p = act_state->sl; p != NULL; p = p->next) {
			assert(p->id != NULL);
			assert(p->state != NULL);

			if (0 == strcmp(p->id, (ZIn))) {
				(ZIs) = p->state;
				break;
			}
		}

		if (p == NULL) {
			struct act_statelist *new;

			new = malloc(sizeof *new);
			if (new == NULL) {
				perror("malloc");
				exit(EXIT_FAILURE);
			}

			new->id = xstrdup((ZIn));
			if (new->id == NULL) {
				perror("xstrdup");
				exit(EXIT_FAILURE);
			}

			(ZIs) = fsm_addstate(fsm);
			if ((ZIs) == NULL) {
				perror("fsm_addstate");
				exit(EXIT_FAILURE);
			}

			new->state = (ZIs);

			new->next = act_state->sl;
			act_state->sl = new;
		}
	
#line 841 "cli/parser.c"
		}
		/* END OF ACTION: add-state */
		/* BEGINNING OF ACTION: mark-end */
		{
#line 148 "cli/parser.act"

		assert((ZIs) != NULL);

		fsm_setend(fsm, (ZIs), 1);
	
#line 852 "cli/parser.c"
		}
		/* END OF ACTION: mark-end */
		/* BEGINNING OF INLINE: 58 */
		{
			switch (CURRENT_TERMINAL) {
			case (lex_comma):
				{
					/* BEGINNING OF INLINE: 34 */
					{
						{
							switch (CURRENT_TERMINAL) {
							case (lex_comma):
								break;
							default:
								goto ZL6;
							}
							ADVANCE_LEXER;
						}
						goto ZL5;
					ZL6:;
						{
							/* BEGINNING OF ACTION: err-expected-comma */
							{
#line 221 "cli/parser.act"

		err_expected("','");
	
#line 880 "cli/parser.c"
							}
							/* END OF ACTION: err-expected-comma */
						}
					ZL5:;
					}
					/* END OF INLINE: 34 */
					/* BEGINNING OF INLINE: ids */
					goto ZL2_ids;
					/* END OF INLINE: ids */
				}
				/*UNREACHED*/
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

void
p_fsm(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 56 */
		{
			switch (CURRENT_TERMINAL) {
			case (lex_ident): case (lex_label):
				{
					string ZIn;
					state ZIs;

					p_things (fsm, lex_state, act_state);
					p_xstart (fsm, lex_state, act_state, &ZIn);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
					/* BEGINNING OF ACTION: add-state */
					{
#line 101 "cli/parser.act"

		struct act_statelist *p;

		assert((ZIn) != NULL);

		for (p = act_state->sl; p != NULL; p = p->next) {
			assert(p->id != NULL);
			assert(p->state != NULL);

			if (0 == strcmp(p->id, (ZIn))) {
				(ZIs) = p->state;
				break;
			}
		}

		if (p == NULL) {
			struct act_statelist *new;

			new = malloc(sizeof *new);
			if (new == NULL) {
				perror("malloc");
				exit(EXIT_FAILURE);
			}

			new->id = xstrdup((ZIn));
			if (new->id == NULL) {
				perror("xstrdup");
				exit(EXIT_FAILURE);
			}

			(ZIs) = fsm_addstate(fsm);
			if ((ZIs) == NULL) {
				perror("fsm_addstate");
				exit(EXIT_FAILURE);
			}

			new->state = (ZIs);

			new->next = act_state->sl;
			act_state->sl = new;
		}
	
#line 970 "cli/parser.c"
					}
					/* END OF ACTION: add-state */
					/* BEGINNING OF ACTION: mark-start */
					{
#line 142 "cli/parser.act"

		assert((ZIs) != NULL);

		fsm_setstart(fsm, (ZIs));
	
#line 981 "cli/parser.c"
					}
					/* END OF ACTION: mark-start */
				}
				break;
			default:
				break;
			}
		}
		/* END OF INLINE: 56 */
		/* BEGINNING OF INLINE: 57 */
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
		/* END OF INLINE: 57 */
		switch (CURRENT_TERMINAL) {
		case (lex_eof):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: free-statelist */
		{
#line 169 "cli/parser.act"

		struct act_statelist *p;
		struct act_statelist *next;

		for (p = act_state->sl; p != NULL; p = next) {
			next = p->next;

			assert(p->id != NULL);

			free(p->id);
			free(p);
		}
	
#line 1031 "cli/parser.c"
		}
		/* END OF ACTION: free-statelist */
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-parse */
		{
#line 238 "cli/parser.act"

		fprintf(stderr, "parse error\n");
		exit(EXIT_FAILURE);
	
#line 1045 "cli/parser.c"
		}
		/* END OF ACTION: err-parse */
	}
}

/* BEGINNING OF TRAILER */

#line 271 "cli/parser.act"


	struct fsm *fsm_parse(struct fsm *fsm, FILE *f) {
		struct act_state act_state_s;
		struct act_state *act_state;
		struct lex_state *lex_state;

		assert(fsm != NULL);
		assert(f != NULL);

		/* TODO: perhaps create the fsm struct here, rather than passing one in */

		act_state_s.sl = NULL;

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

#line 1085 "cli/parser.c"

/* END OF FILE */
