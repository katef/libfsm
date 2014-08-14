/*
 * Automatically generated from the files:
 *	src/fsm/parser.sid
 * and
 *	src/fsm/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 92 "src/fsm/parser.act"


	#include <assert.h>
	#include <stdlib.h>
	#include <string.h>
	#include <stdarg.h>
	#include <stdio.h>

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

	struct lex_state {
		struct lx lx;
		struct lx_dynbuf buf;

		/* TODO: use lx's generated conveniences for the pattern buffer */
		char a[512];
		char *p;
	};

	struct act_statelist {
		char *id;
		struct fsm_state *state;
		struct act_statelist *next;
	};

	#define CURRENT_TERMINAL (act_state->lex_tok)
	#define ERROR_TERMINAL   TOK_UNKNOWN
	#define ADVANCE_LEXER    do { act_state->lex_tok = lx_next(&lex_state->lx); } while (0)
	#define SAVE_LEXER(tok)  do { act_state->lex_tok_save = act_state->lex_tok; \
	                              act_state->lex_tok = tok;                     } while (0)
	#define RESTORE_LEXER    do { act_state->lex_tok = act_state->lex_tok_save; } while (0)

	static void
	err(const struct lex_state *lex_state, const char *fmt, ...)
	{
		va_list ap;

		assert(lex_state != NULL);

		va_start(ap, fmt);
		fprintf(stderr, "%u:%u: ",
			lex_state->lx.start.line, lex_state->lx.start.col);
		vfprintf(stderr, fmt, ap);
		fprintf(stderr, "\n");
		va_end(ap);
	}

	static void
	err_expected(const struct lex_state *lex_state, const char *token)
	{
		err(lex_state, "Syntax error: expected %s", token);
		exit(EXIT_FAILURE);
	}

#line 84 "src/fsm/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_label(fsm, lex_state, act_state, string *);
static void p_edges(fsm, lex_state, act_state);
static void p_edges_C_Cedge(fsm, lex_state, act_state);
static void p_label_C_Cbody(fsm, lex_state, act_state);
static void p_xstart(fsm, lex_state, act_state);
static void p_xend(fsm, lex_state, act_state);
static void p_59(fsm, lex_state, act_state);
extern void p_fsm(fsm, lex_state, act_state);
static void p_id(fsm, lex_state, act_state, string *);
static void p_xend_C_Cend_Hids(fsm, lex_state, act_state);
static void p_xend_C_Cend_Hid(fsm, lex_state, act_state);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_label(fsm fsm, lex_state lex_state, act_state act_state, string *ZOn)
{
	string ZIn;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_label_C_Cbody (fsm, lex_state, act_state);
		switch (CURRENT_TERMINAL) {
		case (TOK_LABEL):
			break;
		case (ERROR_TERMINAL):
			RESTORE_LEXER;
			goto ZL1;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: label-buffer */
		{
#line 136 "src/fsm/parser.act"

		/* TODO */
		*lex_state->p++ = '\0';

		/*
		 * Note we needn't strdup() here only because the grammar does not
		 * permit two adjacent LABELs. If it did, the label buffer would be
		 * overwritten by the LL(1) one-token lookahead.
		 */
		(ZIn) = lex_state->a;

		lex_state->p = lex_state->a;
	
#line 146 "src/fsm/parser.c"
		}
		/* END OF ACTION: label-buffer */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOn = ZIn;
}

static void
p_edges(fsm fsm, lex_state lex_state, act_state act_state)
{
ZL2_edges:;
	switch (CURRENT_TERMINAL) {
	case (TOK_IDENT): case (TOK_CHAR):
		{
			p_edges_C_Cedge (fsm, lex_state, act_state);
			/* BEGINNING OF INLINE: edges */
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			} else {
				goto ZL2_edges;
			}
			/* END OF INLINE: edges */
		}
		/*UNREACHED*/
	case (ERROR_TERMINAL):
		return;
	default:
		break;
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_edges_C_Cedge(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		string ZIa;
		string ZIb;
		state ZIx;
		state ZIy;

		p_id (fsm, lex_state, act_state, &ZIa);
		switch (CURRENT_TERMINAL) {
		case (TOK_TO):
			break;
		case (ERROR_TERMINAL):
			RESTORE_LEXER;
			goto ZL1;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		p_id (fsm, lex_state, act_state, &ZIb);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
		/* BEGINNING OF ACTION: add-state */
		{
#line 144 "src/fsm/parser.act"

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
	
#line 260 "src/fsm/parser.c"
		}
		/* END OF ACTION: add-state */
		/* BEGINNING OF ACTION: add-state */
		{
#line 144 "src/fsm/parser.act"

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
	
#line 308 "src/fsm/parser.c"
		}
		/* END OF ACTION: add-state */
		/* BEGINNING OF INLINE: 45 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: add-edge-any */
					{
#line 230 "src/fsm/parser.act"

		if (!fsm_addedge_any(fsm, (ZIx), (ZIy))) {
			perror("fsm_addedge_any");
			exit(EXIT_FAILURE);
		}
	
#line 326 "src/fsm/parser.c"
					}
					/* END OF ACTION: add-edge-any */
				}
				break;
			case (TOK_CHAR):
				{
					string ZIs;

					p_label (fsm, lex_state, act_state, &ZIs);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL3;
					}
					/* BEGINNING OF ACTION: add-edge-literal */
					{
#line 215 "src/fsm/parser.act"

		assert((ZIs) != NULL);

		/* TODO: convert to single char in the grammar? */
		if (strlen((ZIs)) != 1) {
			err(lex_state, "Edge literals must have exactly one character");
			goto ZL3;
		}

		if (!fsm_addedge_literal(fsm, (ZIx), (ZIy), (ZIs)[0])) {
			perror("fsm_addedge_literal");
			exit(EXIT_FAILURE);
		}
	
#line 357 "src/fsm/parser.c"
					}
					/* END OF ACTION: add-edge-literal */
				}
				break;
			default:
				{
					/* BEGINNING OF ACTION: add-edge-epsilon */
					{
#line 237 "src/fsm/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIy))) {
			perror("fsm_addedge_epsilon");
			exit(EXIT_FAILURE);
		}
	
#line 373 "src/fsm/parser.c"
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
#line 250 "src/fsm/parser.act"

		err_expected(lex_state, "transition");
	
#line 388 "src/fsm/parser.c"
				}
				/* END OF ACTION: err-expected-trans */
			}
		ZL2:;
		}
		/* END OF INLINE: 45 */
		p_59 (fsm, lex_state, act_state);
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
p_label_C_Cbody(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_label_C_Cbody:;
	{
		char ZIc;

		switch (CURRENT_TERMINAL) {
		case (TOK_CHAR):
			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 109 "src/fsm/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIc = lex_state->buf.a[0];
	
#line 428 "src/fsm/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: label-char */
		{
#line 124 "src/fsm/parser.act"

		/* TODO */
		*lex_state->p++ = (ZIc);
	
#line 443 "src/fsm/parser.c"
		}
		/* END OF ACTION: label-char */
		/* BEGINNING OF INLINE: 36 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					/* BEGINNING OF INLINE: label::body */
					goto ZL2_label_C_Cbody;
					/* END OF INLINE: label::body */
				}
				/*UNREACHED*/
			default:
				break;
			}
		}
		/* END OF INLINE: 36 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_xstart(fsm fsm, lex_state lex_state, act_state act_state)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_START):
		{
			string ZIn;
			state ZIs;

			/* BEGINNING OF INLINE: 49 */
			{
				{
					switch (CURRENT_TERMINAL) {
					case (TOK_START):
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
#line 258 "src/fsm/parser.act"

		err_expected(lex_state, "'start:'");
	
#line 497 "src/fsm/parser.c"
					}
					/* END OF ACTION: err-expected-start */
				}
			ZL2:;
			}
			/* END OF INLINE: 49 */
			p_id (fsm, lex_state, act_state, &ZIn);
			p_59 (fsm, lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: add-state */
			{
#line 144 "src/fsm/parser.act"

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
	
#line 555 "src/fsm/parser.c"
			}
			/* END OF ACTION: add-state */
			/* BEGINNING OF ACTION: mark-start */
			{
#line 185 "src/fsm/parser.act"

		assert((ZIs) != NULL);

		fsm_setstart(fsm, (ZIs));
	
#line 566 "src/fsm/parser.c"
			}
			/* END OF ACTION: mark-start */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	default:
		break;
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_xend(fsm fsm, lex_state lex_state, act_state act_state)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_END):
		{
			/* BEGINNING OF INLINE: 58 */
			{
				{
					switch (CURRENT_TERMINAL) {
					case (TOK_END):
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
#line 262 "src/fsm/parser.act"

		err_expected(lex_state, "'end:'");
	
#line 608 "src/fsm/parser.c"
					}
					/* END OF ACTION: err-expected-end */
				}
			ZL2:;
			}
			/* END OF INLINE: 58 */
			p_xend_C_Cend_Hids (fsm, lex_state, act_state);
			p_59 (fsm, lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (ERROR_TERMINAL):
		return;
	default:
		break;
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_59(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case (TOK_SEP):
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
#line 246 "src/fsm/parser.act"

		err_expected(lex_state, "';'");
	
#line 658 "src/fsm/parser.c"
		}
		/* END OF ACTION: err-expected-sep */
	}
}

void
p_fsm(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_edges (fsm, lex_state, act_state);
		p_xstart (fsm, lex_state, act_state);
		p_xend (fsm, lex_state, act_state);
		switch (CURRENT_TERMINAL) {
		case (TOK_EOF):
			break;
		case (ERROR_TERMINAL):
			RESTORE_LEXER;
			goto ZL1;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: free-statelist */
		{
#line 212 "src/fsm/parser.act"

		struct act_statelist *p;
		struct act_statelist *next;

		for (p = act_state->sl; p != NULL; p = next) {
			next = p->next;

			assert(p->id != NULL);

			free(p->id);
			free(p);
		}
	
#line 700 "src/fsm/parser.c"
		}
		/* END OF ACTION: free-statelist */
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-syntax */
		{
#line 267 "src/fsm/parser.act"

		err(lex_state, "Syntax error");
		exit(EXIT_FAILURE);
	
#line 714 "src/fsm/parser.c"
		}
		/* END OF ACTION: err-syntax */
	}
}

static void
p_id(fsm fsm, lex_state lex_state, act_state act_state, string *ZOn)
{
	string ZIn;

	switch (CURRENT_TERMINAL) {
	case (TOK_IDENT):
		{
			/* BEGINNING OF EXTRACT: IDENT */
			{
#line 113 "src/fsm/parser.act"

		ZIn = xstrdup(lex_state->buf.a);
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 738 "src/fsm/parser.c"
			}
			/* END OF EXTRACT: IDENT */
			ADVANCE_LEXER;
		}
		break;
	case (TOK_CHAR):
		{
			p_label (fsm, lex_state, act_state, &ZIn);
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
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOn = ZIn;
}

static void
p_xend_C_Cend_Hids(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_xend_C_Cend_Hids:;
	{
		p_xend_C_Cend_Hid (fsm, lex_state, act_state);
		/* BEGINNING OF INLINE: 56 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_COMMA):
				{
					/* BEGINNING OF INLINE: 57 */
					{
						{
							switch (CURRENT_TERMINAL) {
							case (TOK_COMMA):
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
#line 254 "src/fsm/parser.act"

		err_expected(lex_state, "','");
	
#line 800 "src/fsm/parser.c"
							}
							/* END OF ACTION: err-expected-comma */
						}
					ZL4:;
					}
					/* END OF INLINE: 57 */
					/* BEGINNING OF INLINE: xend::end-ids */
					goto ZL2_xend_C_Cend_Hids;
					/* END OF INLINE: xend::end-ids */
				}
				/*UNREACHED*/
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			default:
				break;
			}
		}
		/* END OF INLINE: 56 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_xend_C_Cend_Hid(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		string ZIn;
		state ZIs;

		p_id (fsm, lex_state, act_state, &ZIn);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
		/* BEGINNING OF ACTION: add-state */
		{
#line 144 "src/fsm/parser.act"

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
	
#line 887 "src/fsm/parser.c"
		}
		/* END OF ACTION: add-state */
		/* BEGINNING OF ACTION: mark-end */
		{
#line 191 "src/fsm/parser.act"

		assert((ZIs) != NULL);

		fsm_setend(fsm, (ZIs), 1);
	
#line 898 "src/fsm/parser.c"
		}
		/* END OF ACTION: mark-end */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

/* BEGINNING OF TRAILER */

#line 318 "src/fsm/parser.act"


	struct fsm *fsm_parse(FILE *f) {
		struct act_state act_state_s;
		struct act_state *act_state;
		struct lex_state lex_state_s;
		struct lex_state *lex_state;
		struct fsm *new;
		struct lx *lx;

		assert(f != NULL);

		lex_state = &lex_state_s;

		lex_state->p = lex_state->a;

		lx = &lex_state->lx;

		lx_init(lx);

		lx->lgetc  = lx_fgetc;
		lx->opaque = f;

		lex_state->buf.a   = NULL;
		lex_state->buf.len = 0;

		lx->buf   = &lex_state->buf;
		lx->push  = lx_dynpush;
		lx->pop   = lx_dynpop;
		lx->clear = lx_dynclear;
		lx->free  = lx_dynfree;

		act_state_s.sl = NULL;

		/* This is a workaround for ADVANCE_LEXER assuming a pointer */
		act_state = &act_state_s;

		new = fsm_new();
		if (new == NULL) {
			perror("fsm_new");
			return NULL;
		}

		ADVANCE_LEXER; /* XXX: what if the first token is unrecognised? */
		p_fsm(new, lex_state, act_state);

		return new;
	}

#line 960 "src/fsm/parser.c"

/* END OF FILE */
