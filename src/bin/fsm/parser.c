/*
 * Automatically generated from the files:
 *	parser.sid
 * and
 *	parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 63 "parser.act"


	#include <assert.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>

	#include <fsm/fsm.h>

	#include <adt/xalloc.h>

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

#line 56 "parser.c"

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_edge(fsm, lex_state, act_state);
static void p_end_Hids(fsm, lex_state, act_state);
static void p_xstart(fsm, lex_state, act_state, string *);
static void p_49(fsm, lex_state, act_state);
static void p_xend(fsm, lex_state, act_state);
static void p_end_Hid(fsm, lex_state, act_state);
extern void p_fsm(fsm, lex_state, act_state);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_edge(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		string ZIa;
		string ZIb;
		state ZIx;
		state ZIy;

		/* BEGINNING OF INLINE: id */
		{
			switch (CURRENT_TERMINAL) {
			case 0:
				{
					/* BEGINNING OF EXTRACT: ident */
					{
#line 80 "parser.act"

		ZIa = xstrdup(lex_tokbuf(lex_state));
		if (ZIa == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 100 "parser.c"
					}
					/* END OF EXTRACT: ident */
					ADVANCE_LEXER;
				}
				break;
			case 1:
				{
					/* BEGINNING OF EXTRACT: label */
					{
#line 88 "parser.act"

		ZIa = xstrdup(lex_tokbuf(lex_state));
		if (ZIa == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 118 "parser.c"
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
		switch (CURRENT_TERMINAL) {
		case 5:
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF INLINE: id */
		{
			switch (CURRENT_TERMINAL) {
			case 0:
				{
					/* BEGINNING OF EXTRACT: ident */
					{
#line 80 "parser.act"

		ZIb = xstrdup(lex_tokbuf(lex_state));
		if (ZIb == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 151 "parser.c"
					}
					/* END OF EXTRACT: ident */
					ADVANCE_LEXER;
				}
				break;
			case 1:
				{
					/* BEGINNING OF EXTRACT: label */
					{
#line 88 "parser.act"

		ZIb = xstrdup(lex_tokbuf(lex_state));
		if (ZIb == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 169 "parser.c"
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
#line 100 "parser.act"

		struct act_statelist *p;

		assert((ZIa) != NULL);

		for (p = act_state->sl; p != NULL; p = p->next) {
			assert(p->id != NULL);
			assert(p->state != NULL);

			if (0 == strcmp(p->id, (ZIa))) {
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

			new->id = xstrdup((ZIa));
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
	
#line 225 "parser.c"
		}
		/* END OF ACTION: add-state */
		/* BEGINNING OF ACTION: add-state */
		{
#line 100 "parser.act"

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
	
#line 273 "parser.c"
		}
		/* END OF ACTION: add-state */
		/* BEGINNING OF INLINE: 48 */
		{
			switch (CURRENT_TERMINAL) {
			case 2:
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: add-edge-any */
					{
#line 193 "parser.act"

		if (!fsm_addedge_any(fsm, (ZIx), (ZIy))) {
			perror("fsm_addedge_any");
			exit(EXIT_FAILURE);
		}
	
#line 291 "parser.c"
					}
					/* END OF ACTION: add-edge-any */
				}
				break;
			case 1:
				{
					string ZIs;

					/* BEGINNING OF EXTRACT: label */
					{
#line 88 "parser.act"

		ZIs = xstrdup(lex_tokbuf(lex_state));
		if (ZIs == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 310 "parser.c"
					}
					/* END OF EXTRACT: label */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: add-edge-literal */
					{
#line 178 "parser.act"

		assert((ZIs) != NULL);

		/* TODO: convert to single char in the grammar? */
		if (strlen((ZIs)) != 1) {
			fprintf(stderr, "edge literals must have exactly one character\n");
			goto ZL5;
		}

		if (!fsm_addedge_literal(fsm, (ZIx), (ZIy), (ZIs)[0])) {
			perror("fsm_addedge_literal");
			exit(EXIT_FAILURE);
		}
	
#line 331 "parser.c"
					}
					/* END OF ACTION: add-edge-literal */
					/* BEGINNING OF ACTION: free */
					{
#line 160 "parser.act"

		free((ZIs));
	
#line 340 "parser.c"
					}
					/* END OF ACTION: free */
				}
				break;
			default:
				{
					/* BEGINNING OF ACTION: add-edge-epsilon */
					{
#line 200 "parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIy))) {
			perror("fsm_addedge_epsilon");
			exit(EXIT_FAILURE);
		}
	
#line 356 "parser.c"
					}
					/* END OF ACTION: add-edge-epsilon */
				}
				break;
			}
			goto ZL4;
		ZL5:;
			{
				/* BEGINNING OF ACTION: err-expected-trans */
				{
#line 213 "parser.act"

		err_expected("transition");
	
#line 371 "parser.c"
				}
				/* END OF ACTION: err-expected-trans */
			}
		ZL4:;
		}
		/* END OF INLINE: 48 */
		p_49 (fsm, lex_state, act_state);
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
		/* BEGINNING OF INLINE: 55 */
		{
			switch (CURRENT_TERMINAL) {
			case 7:
				{
					/* BEGINNING OF INLINE: 36 */
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
#line 217 "parser.act"

		err_expected("','");
	
#line 424 "parser.c"
							}
							/* END OF ACTION: err-expected-comma */
						}
					ZL4:;
					}
					/* END OF INLINE: 36 */
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
		/* END OF INLINE: 55 */
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
		/* BEGINNING OF INLINE: 41 */
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
#line 221 "parser.act"

		err_expected("'start:'");
	
#line 480 "parser.c"
				}
				/* END OF ACTION: err-expected-start */
			}
		ZL2:;
		}
		/* END OF INLINE: 41 */
		/* BEGINNING OF INLINE: id */
		{
			switch (CURRENT_TERMINAL) {
			case 0:
				{
					/* BEGINNING OF EXTRACT: ident */
					{
#line 80 "parser.act"

		ZIn = xstrdup(lex_tokbuf(lex_state));
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 502 "parser.c"
					}
					/* END OF EXTRACT: ident */
					ADVANCE_LEXER;
				}
				break;
			case 1:
				{
					/* BEGINNING OF EXTRACT: label */
					{
#line 88 "parser.act"

		ZIn = xstrdup(lex_tokbuf(lex_state));
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 520 "parser.c"
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
		p_49 (fsm, lex_state, act_state);
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
p_49(fsm fsm, lex_state lex_state, act_state act_state)
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
#line 209 "parser.act"

		err_expected("';'");
	
#line 569 "parser.c"
		}
		/* END OF ACTION: err-expected-sep */
	}
}

static void
p_xend(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == 11) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 38 */
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
#line 225 "parser.act"

		err_expected("'end:'");
	
#line 602 "parser.c"
				}
				/* END OF ACTION: err-expected-end */
			}
		ZL2:;
		}
		/* END OF INLINE: 38 */
		p_end_Hids (fsm, lex_state, act_state);
		p_49 (fsm, lex_state, act_state);
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
		state ZIs;

		/* BEGINNING OF INLINE: id */
		{
			switch (CURRENT_TERMINAL) {
			case 0:
				{
					/* BEGINNING OF EXTRACT: ident */
					{
#line 80 "parser.act"

		ZIn = xstrdup(lex_tokbuf(lex_state));
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 647 "parser.c"
					}
					/* END OF EXTRACT: ident */
					ADVANCE_LEXER;
				}
				break;
			case 1:
				{
					/* BEGINNING OF EXTRACT: label */
					{
#line 88 "parser.act"

		ZIn = xstrdup(lex_tokbuf(lex_state));
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 665 "parser.c"
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
#line 100 "parser.act"

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
	
#line 721 "parser.c"
		}
		/* END OF ACTION: add-state */
		/* BEGINNING OF ACTION: mark-end */
		{
#line 147 "parser.act"

		assert((ZIs) != NULL);

		fsm_setend(fsm, (ZIs), 1);
	
#line 732 "parser.c"
		}
		/* END OF ACTION: mark-end */
		/* BEGINNING OF INLINE: 33 */
		{
			switch (CURRENT_TERMINAL) {
			case 8:
				{
					string ZIc;

					ADVANCE_LEXER;
					switch (CURRENT_TERMINAL) {
					case 1:
						/* BEGINNING OF EXTRACT: label */
						{
#line 88 "parser.act"

		ZIc = xstrdup(lex_tokbuf(lex_state));
		if (ZIc == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 755 "parser.c"
						}
						/* END OF EXTRACT: label */
						break;
					default:
						goto ZL1;
					}
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: add-colour */
					{
#line 153 "parser.act"

		assert((ZIs) != NULL);
		assert((ZIc) != NULL);

		fsm_addcolour(fsm, (ZIs), (ZIc));
	
#line 772 "parser.c"
					}
					/* END OF ACTION: add-colour */
				}
				break;
			default:
				break;
			}
		}
		/* END OF INLINE: 33 */
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
		/* BEGINNING OF INLINE: 56 */
		{
		ZL3_56:;
			switch (CURRENT_TERMINAL) {
			case 0: case 1:
				{
					/* BEGINNING OF INLINE: edges */
					{
						{
							p_edge (fsm, lex_state, act_state);
							/* BEGINNING OF INLINE: 56 */
							if ((CURRENT_TERMINAL) == 11) {
								RESTORE_LEXER;
								goto ZL1;
							} else {
								goto ZL3_56;
							}
							/* END OF INLINE: 56 */
						}
						/*UNREACHED*/
					}
					/* END OF INLINE: edges */
				}
				/*UNREACHED*/
			default:
				break;
			}
		}
		/* END OF INLINE: 56 */
		/* BEGINNING OF INLINE: 53 */
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
#line 100 "parser.act"

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
	
#line 883 "parser.c"
					}
					/* END OF ACTION: add-state */
					/* BEGINNING OF ACTION: mark-start */
					{
#line 141 "parser.act"

		assert((ZIs) != NULL);

		fsm_setstart(fsm, (ZIs));
	
#line 894 "parser.c"
					}
					/* END OF ACTION: mark-start */
				}
				break;
			default:
				break;
			}
		}
		/* END OF INLINE: 53 */
		/* BEGINNING OF INLINE: 54 */
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
		/* END OF INLINE: 54 */
		switch (CURRENT_TERMINAL) {
		case 9:
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: free-statelist */
		{
#line 175 "parser.act"

		struct act_statelist *p;
		struct act_statelist *next;

		for (p = act_state->sl; p != NULL; p = next) {
			next = p->next;

			assert(p->id != NULL);

			free(p->id);
			free(p);
		}
	
#line 944 "parser.c"
		}
		/* END OF ACTION: free-statelist */
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-parse */
		{
#line 230 "parser.act"

		fprintf(stderr, "parse error\n");
		exit(EXIT_FAILURE);
	
#line 958 "parser.c"
		}
		/* END OF ACTION: err-parse */
	}
}

/* BEGINNING OF TRAILER */

#line 267 "parser.act"


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

#line 1002 "parser.c"

/* END OF FILE */
