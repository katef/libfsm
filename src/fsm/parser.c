/*
 * Automatically generated from the files:
 *	src/fsm/parser.sid
 * and
 *	src/fsm/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 93 "src/fsm/parser.act"


	#include <assert.h>
	#include <string.h>
	#include <stdlib.h>
	#include <stdarg.h>
	#include <stdio.h>
	#include <errno.h>

	#include <fsm/fsm.h>

	#include <adt/xalloc.h>

	#include "libfsm/internal.h"	/* XXX */

	#include "lexer.h"
	#include "parser.h"

	typedef char *      string;
	typedef fsm_state_t state;

	struct act_state {
		int lex_tok;
		int lex_tok_save;
		struct act_statelist *sl;
	};

	struct lex_state {
		struct lx lx;
		struct lx_dynbuf buf;
	};

	struct act_statelist {
		char *id;
		fsm_state_t state;
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

#line 81 "src/fsm/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_label(fsm, lex_state, act_state, char *);
static void p_items(fsm, lex_state, act_state);
static void p_xstart(fsm, lex_state, act_state);
static void p_xend(fsm, lex_state, act_state);
static void p_61(fsm, lex_state, act_state);
static void p_63(fsm, lex_state, act_state, string *);
extern void p_fsm(fsm, lex_state, act_state);
static void p_xend_C_Cend_Hids(fsm, lex_state, act_state);
static void p_xend_C_Cend_Hid(fsm, lex_state, act_state);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_label(fsm fsm, lex_state lex_state, act_state act_state, char *ZOc)
{
	char ZIc;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 34 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 182 "src/fsm/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIc = lex_state->buf.a[0];
	
#line 128 "src/fsm/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_ESC):
				{
					/* BEGINNING OF EXTRACT: ESC */
					{
#line 110 "src/fsm/parser.act"

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(2 == strlen(lex_state->buf.a));

		ZIc = lex_state->buf.a[1];

		switch (ZIc) {
		case '\\': ZIc = '\\'; break;
		case '\'': ZIc = '\''; break;
		case 'f':  ZIc = '\f'; break;
		case 'n':  ZIc = '\n'; break;
		case 'r':  ZIc = '\r'; break;
		case 't':  ZIc = '\t'; break;
		case 'v':  ZIc = '\v'; break;
		default:              break;
		}
	
#line 156 "src/fsm/parser.c"
					}
					/* END OF EXTRACT: ESC */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_HEX):
				{
					/* BEGINNING OF EXTRACT: HEX */
					{
#line 175 "src/fsm/parser.act"

		unsigned long u;
		char *e;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 3);

		errno = 0;

		u = strtoul(lex_state->buf.a + 2, &e, 16);
		assert(*e == '\0');

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err(lex_state, "hex escape %s out of range: expected \\x0..\\x%x inclusive",
				lex_state->buf.a, UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err(lex_state, "%s: %s: expected \\x0..\\x%x inclusive",
				lex_state->buf.a, strerror(errno), UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		ZIc = (char) (unsigned char) u;
	
#line 193 "src/fsm/parser.c"
					}
					/* END OF EXTRACT: HEX */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_OCT):
				{
					/* BEGINNING OF EXTRACT: OCT */
					{
#line 148 "src/fsm/parser.act"

		unsigned long u;
		char *e;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		errno = 0;

		u = strtoul(lex_state->buf.a + 1, &e, 8);
		assert(*e == '\0');

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err(lex_state, "octal escape %s out of range: expected \\0..\\%o inclusive",
				lex_state->buf.a, UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err(lex_state, "%s: %s: expected \\0..\\%o inclusive",
				lex_state->buf.a, strerror(errno), UCHAR_MAX);
			exit(EXIT_FAILURE);
		}

		ZIc = (char) (unsigned char) u;
	
#line 230 "src/fsm/parser.c"
					}
					/* END OF EXTRACT: OCT */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 34 */
		switch (CURRENT_TERMINAL) {
		case (TOK_LABEL):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOc = ZIc;
}

static void
p_items(fsm fsm, lex_state lex_state, act_state act_state)
{
ZL2_items:;
	switch (CURRENT_TERMINAL) {
	case (TOK_IDENT):
		{
			/* BEGINNING OF INLINE: 48 */
			{
				{
					string ZIa;

					/* BEGINNING OF INLINE: id */
					{
						{
							switch (CURRENT_TERMINAL) {
							case (TOK_IDENT):
								/* BEGINNING OF EXTRACT: IDENT */
								{
#line 186 "src/fsm/parser.act"

		ZIa = xstrdup(lex_state->buf.a);
		if (ZIa == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 284 "src/fsm/parser.c"
								}
								/* END OF EXTRACT: IDENT */
								break;
							default:
								goto ZL1;
							}
							ADVANCE_LEXER;
						}
					}
					/* END OF INLINE: id */
					p_63 (fsm, lex_state, act_state, &ZIa);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
			}
			/* END OF INLINE: 48 */
			/* BEGINNING OF INLINE: items */
			goto ZL2_items;
			/* END OF INLINE: items */
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
p_xstart(fsm fsm, lex_state lex_state, act_state act_state)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_START):
		{
			string ZIn;
			state ZIs;

			/* BEGINNING OF INLINE: 50 */
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
#line 298 "src/fsm/parser.act"

		err_expected(lex_state, "'start:'");
	
#line 348 "src/fsm/parser.c"
					}
					/* END OF ACTION: err-expected-start */
				}
			ZL2:;
			}
			/* END OF INLINE: 50 */
			/* BEGINNING OF INLINE: id */
			{
				{
					switch (CURRENT_TERMINAL) {
					case (TOK_IDENT):
						/* BEGINNING OF EXTRACT: IDENT */
						{
#line 186 "src/fsm/parser.act"

		ZIn = xstrdup(lex_state->buf.a);
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 370 "src/fsm/parser.c"
						}
						/* END OF EXTRACT: IDENT */
						break;
					default:
						goto ZL1;
					}
					ADVANCE_LEXER;
				}
			}
			/* END OF INLINE: id */
			p_61 (fsm, lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: add-state */
			{
#line 198 "src/fsm/parser.act"

		struct act_statelist *p;

		assert((ZIn) != NULL);

		for (p = act_state->sl; p != NULL; p = p->next) {
			assert(p->id != NULL);

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

			if (!fsm_addstate(fsm, &(ZIs))) {
				perror("fsm_addstate");
				exit(EXIT_FAILURE);
			}

			new->state = (ZIs);

			new->next = act_state->sl;
			act_state->sl = new;
		}
	
#line 429 "src/fsm/parser.c"
			}
			/* END OF ACTION: add-state */
			/* BEGINNING OF ACTION: mark-start */
			{
#line 237 "src/fsm/parser.act"

		fsm_setstart(fsm, (ZIs));
	
#line 438 "src/fsm/parser.c"
			}
			/* END OF ACTION: mark-start */
			/* BEGINNING OF ACTION: free */
			{
#line 245 "src/fsm/parser.act"

		free((ZIn));
	
#line 447 "src/fsm/parser.c"
			}
			/* END OF ACTION: free */
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
			/* BEGINNING OF INLINE: 60 */
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
#line 302 "src/fsm/parser.act"

		err_expected(lex_state, "'end:'");
	
#line 489 "src/fsm/parser.c"
					}
					/* END OF ACTION: err-expected-end */
				}
			ZL2:;
			}
			/* END OF INLINE: 60 */
			p_xend_C_Cend_Hids (fsm, lex_state, act_state);
			p_61 (fsm, lex_state, act_state);
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
p_61(fsm fsm, lex_state lex_state, act_state act_state)
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
#line 286 "src/fsm/parser.act"

		err_expected(lex_state, "';'");
	
#line 539 "src/fsm/parser.c"
		}
		/* END OF ACTION: err-expected-sep */
	}
}

static void
p_63(fsm fsm, lex_state lex_state, act_state act_state, string *ZIa)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_TO):
		{
			string ZIb;
			state ZIx;
			state ZIy;

			ADVANCE_LEXER;
			/* BEGINNING OF INLINE: id */
			{
				{
					switch (CURRENT_TERMINAL) {
					case (TOK_IDENT):
						/* BEGINNING OF EXTRACT: IDENT */
						{
#line 186 "src/fsm/parser.act"

		ZIb = xstrdup(lex_state->buf.a);
		if (ZIb == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 571 "src/fsm/parser.c"
						}
						/* END OF EXTRACT: IDENT */
						break;
					default:
						goto ZL1;
					}
					ADVANCE_LEXER;
				}
			}
			/* END OF INLINE: id */
			/* BEGINNING OF ACTION: add-state */
			{
#line 198 "src/fsm/parser.act"

		struct act_statelist *p;

		assert((*ZIa) != NULL);

		for (p = act_state->sl; p != NULL; p = p->next) {
			assert(p->id != NULL);

			if (0 == strcmp(p->id, (*ZIa))) {
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

			new->id = xstrdup((*ZIa));
			if (new->id == NULL) {
				perror("xstrdup");
				exit(EXIT_FAILURE);
			}

			if (!fsm_addstate(fsm, &(ZIx))) {
				perror("fsm_addstate");
				exit(EXIT_FAILURE);
			}

			new->state = (ZIx);

			new->next = act_state->sl;
			act_state->sl = new;
		}
	
#line 625 "src/fsm/parser.c"
			}
			/* END OF ACTION: add-state */
			/* BEGINNING OF ACTION: add-state */
			{
#line 198 "src/fsm/parser.act"

		struct act_statelist *p;

		assert((ZIb) != NULL);

		for (p = act_state->sl; p != NULL; p = p->next) {
			assert(p->id != NULL);

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

			if (!fsm_addstate(fsm, &(ZIy))) {
				perror("fsm_addstate");
				exit(EXIT_FAILURE);
			}

			new->state = (ZIy);

			new->next = act_state->sl;
			act_state->sl = new;
		}
	
#line 671 "src/fsm/parser.c"
			}
			/* END OF ACTION: add-state */
			/* BEGINNING OF ACTION: free */
			{
#line 245 "src/fsm/parser.act"

		free((*ZIa));
	
#line 680 "src/fsm/parser.c"
			}
			/* END OF ACTION: free */
			/* BEGINNING OF ACTION: free */
			{
#line 245 "src/fsm/parser.act"

		free((ZIb));
	
#line 689 "src/fsm/parser.c"
			}
			/* END OF ACTION: free */
			/* BEGINNING OF INLINE: 42 */
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_ANY):
					{
						ADVANCE_LEXER;
						/* BEGINNING OF ACTION: add-edge-any */
						{
#line 270 "src/fsm/parser.act"

		if (!fsm_addedge_any(fsm, (ZIx), (ZIy))) {
			perror("fsm_addedge_any");
			exit(EXIT_FAILURE);
		}
	
#line 707 "src/fsm/parser.c"
						}
						/* END OF ACTION: add-edge-any */
					}
					break;
				case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
					{
						char ZIc;

						p_label (fsm, lex_state, act_state, &ZIc);
						if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
							RESTORE_LEXER;
							goto ZL4;
						}
						/* BEGINNING OF ACTION: add-edge-literal */
						{
#line 263 "src/fsm/parser.act"

		if (!fsm_addedge_literal(fsm, (ZIx), (ZIy), (ZIc))) {
			perror("fsm_addedge_literal");
			exit(EXIT_FAILURE);
		}
	
#line 730 "src/fsm/parser.c"
						}
						/* END OF ACTION: add-edge-literal */
					}
					break;
				default:
					{
						/* BEGINNING OF ACTION: add-edge-epsilon */
						{
#line 277 "src/fsm/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIy))) {
			perror("fsm_addedge_epsilon");
			exit(EXIT_FAILURE);
		}
	
#line 746 "src/fsm/parser.c"
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
#line 290 "src/fsm/parser.act"

		err_expected(lex_state, "transition");
	
#line 761 "src/fsm/parser.c"
					}
					/* END OF ACTION: err-expected-trans */
				}
			ZL3:;
			}
			/* END OF INLINE: 42 */
			p_61 (fsm, lex_state, act_state);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_SEP):
		{
			state ZI45;

			/* BEGINNING OF ACTION: add-state */
			{
#line 198 "src/fsm/parser.act"

		struct act_statelist *p;

		assert((*ZIa) != NULL);

		for (p = act_state->sl; p != NULL; p = p->next) {
			assert(p->id != NULL);

			if (0 == strcmp(p->id, (*ZIa))) {
				(ZI45) = p->state;
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

			new->id = xstrdup((*ZIa));
			if (new->id == NULL) {
				perror("xstrdup");
				exit(EXIT_FAILURE);
			}

			if (!fsm_addstate(fsm, &(ZI45))) {
				perror("fsm_addstate");
				exit(EXIT_FAILURE);
			}

			new->state = (ZI45);

			new->next = act_state->sl;
			act_state->sl = new;
		}
	
#line 822 "src/fsm/parser.c"
			}
			/* END OF ACTION: add-state */
			/* BEGINNING OF ACTION: free */
			{
#line 245 "src/fsm/parser.act"

		free((*ZIa));
	
#line 831 "src/fsm/parser.c"
			}
			/* END OF ACTION: free */
			p_61 (fsm, lex_state, act_state);
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

void
p_fsm(fsm fsm, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_items (fsm, lex_state, act_state);
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
#line 260 "src/fsm/parser.act"

		struct act_statelist *p;
		struct act_statelist *next;

		for (p = act_state->sl; p != NULL; p = next) {
			next = p->next;

			assert(p->id != NULL);

			free(p->id);
			free(p);
		}
	
#line 888 "src/fsm/parser.c"
		}
		/* END OF ACTION: free-statelist */
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-syntax */
		{
#line 307 "src/fsm/parser.act"

		err(lex_state, "Syntax error");
		exit(EXIT_FAILURE);
	
#line 902 "src/fsm/parser.c"
		}
		/* END OF ACTION: err-syntax */
	}
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
		/* BEGINNING OF INLINE: 58 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_COMMA):
				{
					/* BEGINNING OF INLINE: 59 */
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
#line 294 "src/fsm/parser.act"

		err_expected(lex_state, "','");
	
#line 942 "src/fsm/parser.c"
							}
							/* END OF ACTION: err-expected-comma */
						}
					ZL4:;
					}
					/* END OF INLINE: 59 */
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
		/* END OF INLINE: 58 */
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

		/* BEGINNING OF INLINE: id */
		{
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_IDENT):
					/* BEGINNING OF EXTRACT: IDENT */
					{
#line 186 "src/fsm/parser.act"

		ZIn = xstrdup(lex_state->buf.a);
		if (ZIn == NULL) {
			perror("xstrdup");
			exit(EXIT_FAILURE);
		}
	
#line 994 "src/fsm/parser.c"
					}
					/* END OF EXTRACT: IDENT */
					break;
				default:
					goto ZL1;
				}
				ADVANCE_LEXER;
			}
		}
		/* END OF INLINE: id */
		/* BEGINNING OF ACTION: add-state */
		{
#line 198 "src/fsm/parser.act"

		struct act_statelist *p;

		assert((ZIn) != NULL);

		for (p = act_state->sl; p != NULL; p = p->next) {
			assert(p->id != NULL);

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

			if (!fsm_addstate(fsm, &(ZIs))) {
				perror("fsm_addstate");
				exit(EXIT_FAILURE);
			}

			new->state = (ZIs);

			new->next = act_state->sl;
			act_state->sl = new;
		}
	
#line 1048 "src/fsm/parser.c"
		}
		/* END OF ACTION: add-state */
		/* BEGINNING OF ACTION: mark-end */
		{
#line 241 "src/fsm/parser.act"

		fsm_setend(fsm, (ZIs), 1);
	
#line 1057 "src/fsm/parser.c"
		}
		/* END OF ACTION: mark-end */
		/* BEGINNING OF ACTION: free */
		{
#line 245 "src/fsm/parser.act"

		free((ZIn));
	
#line 1066 "src/fsm/parser.c"
		}
		/* END OF ACTION: free */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

/* BEGINNING OF TRAILER */

#line 359 "src/fsm/parser.act"


	struct fsm *fsm_parse(FILE *f, const struct fsm_options *opt) {
		struct act_state act_state_s;
		struct act_state *act_state;
		struct lex_state lex_state_s;
		struct lex_state *lex_state;
		struct fsm *new;
		struct lx *lx;

		assert(f != NULL);
		assert(opt != NULL);

		lex_state = &lex_state_s;

		lx = &lex_state->lx;

		lx_init(lx);

		lx->lgetc       = lx_fgetc;
		lx->getc_opaque = f;

		lex_state->buf.a   = NULL;
		lex_state->buf.len = 0;

		lx->buf_opaque = &lex_state->buf;
		lx->push       = lx_dynpush;
		lx->clear      = lx_dynclear;
		lx->free       = lx_dynfree;

		act_state_s.sl = NULL;

		/* This is a workaround for ADVANCE_LEXER assuming a pointer */
		act_state = &act_state_s;

		/* TODO: pass in fsm_options */
		new = fsm_new(opt);
		if (new == NULL) {
			perror("fsm_new");
			return NULL;
		}

		ADVANCE_LEXER; /* XXX: what if the first token is unrecognised? */
		p_fsm(new, lex_state, act_state);

		lx->free(lx->buf_opaque);

		return new;
	}

#line 1129 "src/fsm/parser.c"

/* END OF FILE */
