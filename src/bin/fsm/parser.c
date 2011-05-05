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
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>

	#include <fsm/fsm.h>
	#include <fsm/colour.h>

	#include "libfsm/xalloc.h"
	#include "libfsm/internal.h"	/* XXX */

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

#line 57 "parser.c"

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_things_C_Cthing(fsm, lex_state, act_state);
static void p_end_Hids(fsm, lex_state, act_state);
static void p_xstart(fsm, lex_state, act_state, string *);
static void p_xend(fsm, lex_state, act_state);
static void p_end_Hid(fsm, lex_state, act_state);
static void p_55(fsm, lex_state, act_state);
static void p_66(fsm, lex_state, act_state, string *);
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
		string ZI65;

		/* BEGINNING OF INLINE: id */
		{
			switch (CURRENT_TERMINAL) {
			case 0:
				{
					/* BEGINNING OF EXTRACT: ident */
					{
#line 81 "parser.act"

		ZI65 = xstrdup(lex_tokbuf(lex_state));
		if (ZI65 == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 99 "parser.c"
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

		ZI65 = xstrdup(lex_tokbuf(lex_state));
		if (ZI65 == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 117 "parser.c"
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
		p_66 (fsm, lex_state, act_state, &ZI65);
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
p_end_Hids(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
ZL2_end_Hids:;
	{
		p_end_Hid (fsm, lex_state, act_state);
		/* BEGINNING OF INLINE: 63 */
		{
			switch (CURRENT_TERMINAL) {
			case 7:
				{
					/* BEGINNING OF INLINE: 38 */
					{
						{
							switch (CURRENT_TERMINAL) {
							case 7:
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
#line 231 "parser.act"

		err_expected("','");
	
#line 174 "parser.c"
							}
							/* END OF ACTION: err-expected-comma */
						}
					ZL4:;
					}
					/* END OF INLINE: 38 */
					/* BEGINNING OF INLINE: end-ids */
					goto ZL2_end_Hids;
					/* END OF INLINE: end-ids */
				}
				/*UNREACHED*/
			case 11:
				RESTORE_LEXER;
				goto ZL1;
			default:
				break;
			}
		}
		/* END OF INLINE: 63 */
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
		/* BEGINNING OF INLINE: 43 */
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
#line 239 "parser.act"

		err_expected("'start:'");
	
#line 230 "parser.c"
				}
				/* END OF ACTION: err-expected-start */
			}
		ZL2:;
		}
		/* END OF INLINE: 43 */
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
	
#line 252 "parser.c"
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
	
#line 270 "parser.c"
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
		p_55 (fsm, lex_state, act_state);
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
		/* BEGINNING OF INLINE: 40 */
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
#line 243 "parser.act"

		err_expected("'end:'");
	
#line 322 "parser.c"
				}
				/* END OF ACTION: err-expected-end */
			}
		ZL2:;
		}
		/* END OF INLINE: 40 */
		p_end_Hids (fsm, lex_state, act_state);
		p_55 (fsm, lex_state, act_state);
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
p_end_Hid(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		string ZIn;
		string ZIc;
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
	
#line 368 "parser.c"
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
	
#line 386 "parser.c"
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
		/* BEGINNING OF INLINE: 34 */
		{
			switch (CURRENT_TERMINAL) {
			case 8:
				{
					ADVANCE_LEXER;
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
	
#line 415 "parser.c"
						}
						/* END OF EXTRACT: label */
						break;
					default:
						goto ZL1;
					}
					ADVANCE_LEXER;
				}
				break;
			default:
				{
					/* BEGINNING OF ACTION: no-colour */
					{
#line 154 "parser.act"

		(ZIc) = NULL;
	
#line 433 "parser.c"
					}
					/* END OF ACTION: no-colour */
				}
				break;
			}
		}
		/* END OF INLINE: 34 */
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
	
#line 486 "parser.c"
		}
		/* END OF ACTION: add-state */
		/* BEGINNING OF ACTION: mark-end */
		{
#line 148 "parser.act"

		assert((ZIs) != NULL);

		fsm_addend(fsm, (ZIs), (ZIc));
	
#line 497 "parser.c"
		}
		/* END OF ACTION: mark-end */
	}
	return;
ZL1:;
	SAVE_LEXER (11);
	return;
}

static void
p_55(fsm fsm, lex_state lex_state, act_state act_state)
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
#line 223 "parser.act"

		err_expected("';'");
	
#line 531 "parser.c"
		}
		/* END OF ACTION: err-expected-sep */
	}
}

static void
p_66(fsm fsm, lex_state lex_state, act_state act_state, string *ZI65)
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
	
#line 563 "parser.c"
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
	
#line 581 "parser.c"
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

		assert((*ZI65) != NULL);

		for (p = act_state->sl; p != NULL; p = p->next) {
			assert(p->id != NULL);
			assert(p->state != NULL);

			if (0 == strcmp(p->id, (*ZI65))) {
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

			new->id = xstrdup((*ZI65));
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
	
#line 637 "parser.c"
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
	
#line 685 "parser.c"
			}
			/* END OF ACTION: add-state */
			/* BEGINNING OF INLINE: 50 */
			{
				switch (CURRENT_TERMINAL) {
				case 2:
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: add-edge-any */
						{
#line 207 "parser.act"

		if (!fsm_addedge_any(fsm, (ZIx), (ZIy))) {
			perror("fsm_addedge_any");
			exit(EXIT_FAILURE);
		}
	
#line 703 "parser.c"
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
	
#line 722 "parser.c"
						}
						/* END OF EXTRACT: label */
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: add-edge-literal */
						{
#line 192 "parser.act"

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
	
#line 743 "parser.c"
						}
						/* END OF ACTION: add-edge-literal */
						/* BEGINNING OF ACTION: free */
						{
#line 158 "parser.act"

		free((ZIs));
	
#line 752 "parser.c"
						}
						/* END OF ACTION: free */
					}
					break;
				default:
					{
						/* BEGINNING OF ACTION: add-edge-epsilon */
						{
#line 214 "parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIy))) {
			perror("fsm_addedge_epsilon");
			exit(EXIT_FAILURE);
		}
	
#line 768 "parser.c"
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
#line 227 "parser.act"

		err_expected("transition");
	
#line 783 "parser.c"
					}
					/* END OF ACTION: err-expected-trans */
				}
			ZL3:;
			}
			/* END OF INLINE: 50 */
			p_55 (fsm, lex_state, act_state);
			if ((CURRENT_TERMINAL) == 11) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case 1:
		{
			state ZIs;
			string ZIl;
			string ZIc;

			/* BEGINNING OF ACTION: add-state */
			{
#line 101 "parser.act"

		struct act_statelist *p;

		assert((*ZI65) != NULL);

		for (p = act_state->sl; p != NULL; p = p->next) {
			assert(p->id != NULL);
			assert(p->state != NULL);

			if (0 == strcmp(p->id, (*ZI65))) {
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

			new->id = xstrdup((*ZI65));
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
	
#line 848 "parser.c"
			}
			/* END OF ACTION: add-state */
			/* BEGINNING OF EXTRACT: label */
			{
#line 89 "parser.act"

		ZIl = xstrdup(lex_tokbuf(lex_state));
		if (ZIl == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 861 "parser.c"
			}
			/* END OF EXTRACT: label */
			ADVANCE_LEXER;
			/* BEGINNING OF INLINE: 54 */
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
#line 235 "parser.act"

		err_expected("'='");
	
#line 885 "parser.c"
					}
					/* END OF ACTION: err-expected-equals */
				}
			ZL5:;
			}
			/* END OF INLINE: 54 */
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
	
#line 904 "parser.c"
				}
				/* END OF EXTRACT: label */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			p_55 (fsm, lex_state, act_state);
			if ((CURRENT_TERMINAL) == 11) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: mark-colour */
			{
#line 176 "parser.act"

		assert((ZIs) != NULL);
		assert((ZIc) != NULL);

		/* TODO: convert to single char in the grammar? */
		if (strlen((ZIl)) != 1) {
			fprintf(stderr, "edge literals must have exactly one character\n");
			goto ZL1;
		}

		if (!fsm_addedgecolour(fsm, &(ZIs)->edges[(unsigned char) (ZIl)[0]], (ZIc))) {
			perror("fsm_addedgecolour");
			goto ZL1;
		}
	
#line 935 "parser.c"
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

void
p_fsm(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 64 */
		{
		ZL3_64:;
			switch (CURRENT_TERMINAL) {
			case 0: case 1:
				{
					/* BEGINNING OF INLINE: things */
					{
						{
							p_things_C_Cthing (fsm, lex_state, act_state);
							/* BEGINNING OF INLINE: 64 */
							if ((CURRENT_TERMINAL) == 11) {
								RESTORE_LEXER;
								goto ZL1;
							} else {
								goto ZL3_64;
							}
							/* END OF INLINE: 64 */
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
		/* END OF INLINE: 64 */
		/* BEGINNING OF INLINE: 61 */
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
	
#line 1045 "parser.c"
					}
					/* END OF ACTION: add-state */
					/* BEGINNING OF ACTION: mark-start */
					{
#line 142 "parser.act"

		assert((ZIs) != NULL);

		fsm_setstart(fsm, (ZIs));
	
#line 1056 "parser.c"
					}
					/* END OF ACTION: mark-start */
				}
				break;
			default:
				break;
			}
		}
		/* END OF INLINE: 61 */
		/* BEGINNING OF INLINE: 62 */
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
		/* END OF INLINE: 62 */
		switch (CURRENT_TERMINAL) {
		case 9:
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: free-statelist */
		{
#line 173 "parser.act"

		struct act_statelist *p;
		struct act_statelist *next;

		for (p = act_state->sl; p != NULL; p = next) {
			next = p->next;

			assert(p->id != NULL);

			free(p->id);
			free(p);
		}
	
#line 1106 "parser.c"
		}
		/* END OF ACTION: free-statelist */
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-parse */
		{
#line 248 "parser.act"

		fprintf(stderr, "parse error\n");
		exit(EXIT_FAILURE);
	
#line 1120 "parser.c"
		}
		/* END OF ACTION: err-parse */
	}
}

/* BEGINNING OF TRAILER */

#line 285 "parser.act"


	struct fsm *fsm_parse(FILE *f) {
		struct act_state act_state_s;
		struct act_state *act_state;
		struct lex_state *lex_state;
		struct fsm *new;

		assert(f != NULL);

		act_state_s.sl = NULL;

		lex_state = lex_init(f);
		if (lex_state == NULL) {
			perror("lex_init");
			return NULL;
		}

		/* This is a workaround for ADVANCE_LEXER assuming a pointer */
		act_state = &act_state_s;

		new = fsm_new();
		if (new == NULL) {
			perror("fsm_new");
			return NULL;
		}

		ADVANCE_LEXER;
		p_fsm(new, lex_state, act_state);

		lex_free(lex_state);

		return new;
	}

#line 1164 "parser.c"

/* END OF FILE */
