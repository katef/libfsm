/*
 * Automatically generated from the files:
 *	parser.sid
 * and
 *	parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 62 "parser.act"


	#include <assert.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>

	#include <fsm/fsm.h>

	#include "libfsm/xalloc.h"

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

#line 55 "parser.c"

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_things_C_Cthing(fsm, lex_state, act_state);
static void p_xstart(fsm, lex_state, act_state, string *);
static void p_xend(fsm, lex_state, act_state);
static void p_51(fsm, lex_state, act_state);
static void p_62(fsm, lex_state, act_state, string *);
static void p_ids(fsm, lex_state, act_state);
extern void p_fsm(fsm, lex_state, act_state);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_things_C_Cthing(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		string ZI61;

		/* BEGINNING OF INLINE: id */
		{
			switch (CURRENT_TERMINAL) {
			case 0:
				{
					/* BEGINNING OF EXTRACT: ident */
					{
#line 81 "parser.act"

		ZI61 = xstrdup(lex_tokbuf(lex_state));
		if (ZI61 == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 96 "parser.c"
					}
					/* END OF EXTRACT: ident */
					ADVANCE_LEXER;
				}
				break;
			case 1:
				{
					/* BEGINNING OF EXTRACT: label */
					{
#line 89 "parser.act"

		ZI61 = xstrdup(lex_tokbuf(lex_state));
		if (ZI61 == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 114 "parser.c"
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
		p_62 (fsm, lex_state, act_state, &ZI61);
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
p_xstart(fsm fsm, lex_state lex_state, act_state act_state, string *ZOn)
{
	string ZIn;

	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 39 */
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
				/* BEGINNING OF ACTION: err-expected-start */
				{
#line 229 "parser.act"

		err_expected("'start:'");
	
#line 166 "parser.c"
				}
				/* END OF ACTION: err-expected-start */
			}
		ZL2:;
		}
		/* END OF INLINE: 39 */
		/* BEGINNING OF INLINE: id */
		{
			switch (CURRENT_TERMINAL) {
			case 0:
				{
					/* BEGINNING OF EXTRACT: ident */
					{
#line 81 "parser.act"

		ZIn = xstrdup(lex_tokbuf(lex_state));
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 188 "parser.c"
					}
					/* END OF EXTRACT: ident */
					ADVANCE_LEXER;
				}
				break;
			case 1:
				{
					/* BEGINNING OF EXTRACT: label */
					{
#line 89 "parser.act"

		ZIn = xstrdup(lex_tokbuf(lex_state));
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 206 "parser.c"
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
	*ZOn = ZIn;
}

static void
p_xend(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 36 */
		{
			{
				switch (CURRENT_TERMINAL) {
				case 4:
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
#line 233 "parser.act"

		err_expected("'end:'");
	
#line 258 "parser.c"
				}
				/* END OF ACTION: err-expected-end */
			}
		ZL2:;
		}
		/* END OF INLINE: 36 */
		p_ids (fsm, lex_state, act_state);
		p_51 (fsm, lex_state, act_state);
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
p_51(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case 6:
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
#line 213 "parser.act"

		err_expected("';'");
	
#line 302 "parser.c"
		}
		/* END OF ACTION: err-expected-sep */
	}
}

static void
p_62(fsm fsm, lex_state lex_state, act_state act_state, string *ZI61)
{
	switch (CURRENT_TERMINAL) {
	case 5:
		{
			string ZIb;
			state ZIx;
			state ZIy;

			ADVANCE_LEXER;
			/* BEGINNING OF INLINE: id */
			{
				switch (CURRENT_TERMINAL) {
				case 0:
					{
						/* BEGINNING OF EXTRACT: ident */
						{
#line 81 "parser.act"

		ZIb = xstrdup(lex_tokbuf(lex_state));
		if (ZIb == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 334 "parser.c"
						}
						/* END OF EXTRACT: ident */
						ADVANCE_LEXER;
					}
					break;
				case 1:
					{
						/* BEGINNING OF EXTRACT: label */
						{
#line 89 "parser.act"

		ZIb = xstrdup(lex_tokbuf(lex_state));
		if (ZIb == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 352 "parser.c"
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
#line 101 "parser.act"

		struct act_statelist *p;

		assert((*ZI61) != NULL);

		for (p = act_state->sl; p != NULL; p = p->next) {
			assert(p->id != NULL);
			assert(p->state != NULL);

			if (0 == strcmp(p->id, (*ZI61))) {
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

			new->id = xstrdup((*ZI61));
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
	
#line 408 "parser.c"
			}
			/* END OF ACTION: add-state */
			/* BEGINNING OF ACTION: add-state */
			{
#line 101 "parser.act"

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
	
#line 456 "parser.c"
			}
			/* END OF ACTION: add-state */
			/* BEGINNING OF INLINE: 46 */
			{
				switch (CURRENT_TERMINAL) {
				case 2:
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: add-edge-any */
						{
#line 197 "parser.act"

		if (!fsm_addedge_any(fsm, (ZIx), (ZIy))) {
			perror("fsm_addedge_any");
			exit(EXIT_FAILURE);
		}
	
#line 474 "parser.c"
						}
						/* END OF ACTION: add-edge-any */
					}
					break;
				case 1:
					{
						string ZIs;

						/* BEGINNING OF EXTRACT: label */
						{
#line 89 "parser.act"

		ZIs = xstrdup(lex_tokbuf(lex_state));
		if (ZIs == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 493 "parser.c"
						}
						/* END OF EXTRACT: label */
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: add-edge-literal */
						{
#line 182 "parser.act"

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
	
#line 514 "parser.c"
						}
						/* END OF ACTION: add-edge-literal */
						/* BEGINNING OF ACTION: free */
						{
#line 154 "parser.act"

		free((ZIs));
	
#line 523 "parser.c"
						}
						/* END OF ACTION: free */
					}
					break;
				default:
					{
						/* BEGINNING OF ACTION: add-edge-epsilon */
						{
#line 204 "parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIy))) {
			perror("fsm_addedge_epsilon");
			exit(EXIT_FAILURE);
		}
	
#line 539 "parser.c"
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
#line 217 "parser.act"

		err_expected("transition");
	
#line 554 "parser.c"
					}
					/* END OF ACTION: err-expected-trans */
				}
			ZL3:;
			}
			/* END OF INLINE: 46 */
			p_51 (fsm, lex_state, act_state);
			if ((CURRENT_TERMINAL) == 11) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case 8:
		{
			state ZIs;
			string ZIc;

			/* BEGINNING OF ACTION: add-state */
			{
#line 101 "parser.act"

		struct act_statelist *p;

		assert((*ZI61) != NULL);

		for (p = act_state->sl; p != NULL; p = p->next) {
			assert(p->id != NULL);
			assert(p->state != NULL);

			if (0 == strcmp(p->id, (*ZI61))) {
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

			new->id = xstrdup((*ZI61));
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
	
#line 618 "parser.c"
			}
			/* END OF ACTION: add-state */
			/* BEGINNING OF INLINE: 49 */
			{
				{
					switch (CURRENT_TERMINAL) {
					case 8:
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
#line 225 "parser.act"

		err_expected("'='");
	
#line 641 "parser.c"
					}
					/* END OF ACTION: err-expected-equals */
				}
			ZL5:;
			}
			/* END OF INLINE: 49 */
			switch (CURRENT_TERMINAL) {
			case 1:
				/* BEGINNING OF EXTRACT: label */
				{
#line 89 "parser.act"

		ZIc = xstrdup(lex_tokbuf(lex_state));
		if (ZIc == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 660 "parser.c"
				}
				/* END OF EXTRACT: label */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			p_51 (fsm, lex_state, act_state);
			if ((CURRENT_TERMINAL) == 11) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: mark-colour */
			{
#line 172 "parser.act"

		assert((ZIs) != NULL);
		assert((ZIc) != NULL);

		if (!fsm_addcolour(fsm, (ZIs), (ZIc))) {
			perror("fsm_addcolour");
			goto ZL1;
		}
	
#line 685 "parser.c"
			}
			/* END OF ACTION: mark-colour */
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

static void
p_ids(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
ZL2_ids:;
	{
		string ZIn;
		state ZIs;

		/* BEGINNING OF INLINE: id */
		{
			switch (CURRENT_TERMINAL) {
			case 0:
				{
					/* BEGINNING OF EXTRACT: ident */
					{
#line 81 "parser.act"

		ZIn = xstrdup(lex_tokbuf(lex_state));
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 727 "parser.c"
					}
					/* END OF EXTRACT: ident */
					ADVANCE_LEXER;
				}
				break;
			case 1:
				{
					/* BEGINNING OF EXTRACT: label */
					{
#line 89 "parser.act"

		ZIn = xstrdup(lex_tokbuf(lex_state));
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 745 "parser.c"
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
#line 101 "parser.act"

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
	
#line 801 "parser.c"
		}
		/* END OF ACTION: add-state */
		/* BEGINNING OF ACTION: mark-end */
		{
#line 148 "parser.act"

		assert((ZIs) != NULL);

		fsm_setend(fsm, (ZIs), 1);
	
#line 812 "parser.c"
		}
		/* END OF ACTION: mark-end */
		/* BEGINNING OF INLINE: 59 */
		{
			switch (CURRENT_TERMINAL) {
			case 7:
				{
					/* BEGINNING OF INLINE: 34 */
					{
						{
							switch (CURRENT_TERMINAL) {
							case 7:
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
#line 221 "parser.act"

		err_expected("','");
	
#line 840 "parser.c"
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
		/* END OF INLINE: 59 */
	}
	return;
ZL1:;
	SAVE_LEXER (11);
	return;
}

void
p_fsm(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 60 */
		{
		ZL3_60:;
			switch (CURRENT_TERMINAL) {
			case 0: case 1:
				{
					/* BEGINNING OF INLINE: things */
					{
						{
							p_things_C_Cthing (fsm, lex_state, act_state);
							/* BEGINNING OF INLINE: 60 */
							if ((CURRENT_TERMINAL) == 11) {
								RESTORE_LEXER;
								goto ZL1;
							} else {
								goto ZL3_60;
							}
							/* END OF INLINE: 60 */
						}
						/*UNREACHED*/
					}
					/* END OF INLINE: things */
				}
				/*UNREACHED*/
			default:
				break;
			}
		}
		/* END OF INLINE: 60 */
		/* BEGINNING OF INLINE: 57 */
		{
			switch (CURRENT_TERMINAL) {
			case 3:
				{
					string ZIn;
					state ZIs;

					p_xstart (fsm, lex_state, act_state, &ZIn);
					if ((CURRENT_TERMINAL) == 11) {
						RESTORE_LEXER;
						goto ZL1;
					}
					/* BEGINNING OF ACTION: add-state */
					{
#line 101 "parser.act"

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
	
#line 958 "parser.c"
					}
					/* END OF ACTION: add-state */
					/* BEGINNING OF ACTION: mark-start */
					{
#line 142 "parser.act"

		assert((ZIs) != NULL);

		fsm_setstart(fsm, (ZIs));
	
#line 969 "parser.c"
					}
					/* END OF ACTION: mark-start */
				}
				break;
			default:
				break;
			}
		}
		/* END OF INLINE: 57 */
		/* BEGINNING OF INLINE: 58 */
		{
			switch (CURRENT_TERMINAL) {
			case 4:
				{
					p_xend (fsm, lex_state, act_state);
					if ((CURRENT_TERMINAL) == 11) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				break;
			}
		}
		/* END OF INLINE: 58 */
		switch (CURRENT_TERMINAL) {
		case 9:
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: free-statelist */
		{
#line 169 "parser.act"

		struct act_statelist *p;
		struct act_statelist *next;

		for (p = act_state->sl; p != NULL; p = next) {
			next = p->next;

			assert(p->id != NULL);

			free(p->id);
			free(p);
		}
	
#line 1019 "parser.c"
		}
		/* END OF ACTION: free-statelist */
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-parse */
		{
#line 238 "parser.act"

		fprintf(stderr, "parse error\n");
		exit(EXIT_FAILURE);
	
#line 1033 "parser.c"
		}
		/* END OF ACTION: err-parse */
	}
}

/* BEGINNING OF TRAILER */

#line 271 "parser.act"


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

#line 1073 "parser.c"

/* END OF FILE */
