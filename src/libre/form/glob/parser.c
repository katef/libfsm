/*
 * Automatically generated from the files:
 *	src/libre/form/glob/parser.sid
 * and
 *	src/libre/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 89 "src/libre/parser.act"


	#include <assert.h>
	#include <limits.h>
	#include <string.h>
	#include <stdlib.h>
	#include <stdarg.h>
	#include <stdio.h>
	#include <errno.h>

	#include "libfsm/internal.h" /* XXX */

	#include <adt/bitmap.h>

	#include <fsm/fsm.h>

	#include <re/re.h>

	#ifndef FORM
	#error FORM required
	#endif

	#define PASTE(a, b) a ## b
	#define CAT(a, b)   PASTE(a, b)

	#define LX_PREFIX CAT(lx_, FORM)

	#define LX_TOKEN  CAT(LX_PREFIX, _token)
	#define LX_STATE  CAT(LX_PREFIX, _lx)
	#define LX_NEXT   CAT(LX_PREFIX, _next)
	#define LX_INIT   CAT(LX_PREFIX, _init)

	#define FORM_COMP CAT(comp_, FORM)

	/* XXX: get rid of this; use same %entry% for all grammars */
	#define FORM_ENTRY CAT(p_re__, FORM)

	#include "parser.h"
	#include "lexer.h"

	#include "../comp.h"

	typedef char        t_char;
	typedef unsigned    t_unsigned;
	typedef unsigned    t_pred; /* TODO */

	typedef struct fsm_state * t_fsm__state;

	typedef struct {
		struct bm set;
	} t_grp;

	struct act_state {
		enum LX_TOKEN lex_tok;
		enum LX_TOKEN lex_tok_save;
		enum re_errno e;
	};

	struct lex_state {
		struct LX_STATE lx;
		struct lx_dynbuf buf; /* XXX: unneccessary since we're lexing from a string */

		int (*f)(void *opaque);
		void *opaque;

		/* TODO: use lx's generated conveniences for the pattern buffer */
		char a[512];
		char *p;
	};

	#define CURRENT_TERMINAL (act_state->lex_tok)
	#define ERROR_TERMINAL   (TOK_ERROR)
	#define ADVANCE_LEXER    do { act_state->lex_tok = LX_NEXT(&lex_state->lx); } while (0)
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

	/* TODO: centralise */
	/* XXX: escaping really depends on dialect */
	static const char *
	escchar(char *s, size_t sz, int c)
	{
		size_t i;

		const struct {
			int c;
			const char *s;
		} a[] = {
			{ '\\', "\\\\" },

			{ '^',  "\\^"  },
			{ '-',  "\\-"  },
			{ ']',  "\\]"  },
			{ '[',  "\\["  },

			{ '\f', "\\f"  },
			{ '\n', "\\n"  },
			{ '\r', "\\r"  },
			{ '\t', "\\t"  },
			{ '\v', "\\v"  }
		};

		assert(s != NULL);
		assert(sz >= 5);

		(void) sz;

		for (i = 0; i < sizeof a / sizeof *a; i++) {
			if (a[i].c == c) {
				return a[i].s;
			}
		}

		if (!isprint((unsigned char) c)) {
			sprintf(s, "\\x%02X", (unsigned char) c);
			return s;
		}

		sprintf(s, "%c", c);
		return s;
	}

#line 151 "src/libre/form/glob/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Hatoms_C_Cliteral(fsm, flags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_list_Hof_Hatoms(fsm, flags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_list_Hof_Hatoms_C_Catom(fsm, flags, lex_state, act_state, t_fsm__state, t_fsm__state *);
static void p_list_Hof_Hatoms_C_Cany(fsm, flags, lex_state, act_state, t_fsm__state, t_fsm__state);
static void p_list_Hof_Hatoms_C_Cwildcard(fsm, flags, lex_state, act_state, t_fsm__state, t_fsm__state);
extern void p_re__glob(fsm, flags, lex_state, act_state);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Hatoms_C_Cliteral(fsm fsm, flags flags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZIc;

		switch (CURRENT_TERMINAL) {
		case (TOK_CHAR):
			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 256 "src/libre/parser.act"

		assert(lex_state->buf.a[0] != '\0');
		assert(lex_state->buf.a[1] == '\0');

		ZIc = lex_state->buf.a[0];
	
#line 192 "src/libre/form/glob/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: add-literal */
		{
#line 417 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		/* TODO: check c is legal? */

		if (!fsm_addedge_literal(fsm, (ZIx), (ZIy), (ZIc))) {
			goto ZL1;
		}
	
#line 213 "src/libre/form/glob/parser.c"
		}
		/* END OF ACTION: add-literal */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Hatoms(fsm fsm, flags flags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_list_Hof_Hatoms:;
	{
		t_fsm__state ZIz;

		/* BEGINNING OF ACTION: add-concat */
		{
#line 393 "src/libre/parser.act"

		(ZIz) = fsm_addstate(fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 242 "src/libre/form/glob/parser.c"
		}
		/* END OF ACTION: add-concat */
		p_list_Hof_Hatoms_C_Catom (fsm, flags, lex_state, act_state, ZIx, &ZIz);
		/* BEGINNING OF INLINE: 64 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_QMARK): case (TOK_STAR): case (TOK_CHAR):
				{
					/* BEGINNING OF INLINE: list-of-atoms */
					ZIx = ZIz;
					goto ZL2_list_Hof_Hatoms;
					/* END OF INLINE: list-of-atoms */
				}
				/* UNREACHED */
			default:
				{
					/* BEGINNING OF ACTION: add-epsilon */
					{
#line 400 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIz), (ZIy))) {
			goto ZL1;
		}
	
#line 267 "src/libre/form/glob/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 64 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Hatoms_C_Catom(fsm fsm, flags flags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state *ZIy)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_QMARK):
		{
			p_list_Hof_Hatoms_C_Cany (fsm, flags, lex_state, act_state, ZIx, *ZIy);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: count-1 */
			{
#line 551 "src/libre/parser.act"

		(void) (ZIx);
		(void) (*ZIy);
	
#line 303 "src/libre/form/glob/parser.c"
			}
			/* END OF ACTION: count-1 */
		}
		break;
	case (TOK_CHAR):
		{
			p_list_Hof_Hatoms_C_Cliteral (fsm, flags, lex_state, act_state, ZIx, *ZIy);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: count-1 */
			{
#line 551 "src/libre/parser.act"

		(void) (ZIx);
		(void) (*ZIy);
	
#line 322 "src/libre/form/glob/parser.c"
			}
			/* END OF ACTION: count-1 */
		}
		break;
	case (TOK_STAR):
		{
			p_list_Hof_Hatoms_C_Cwildcard (fsm, flags, lex_state, act_state, ZIx, *ZIy);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: count-0-or-many */
			{
#line 501 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (*ZIy))) {
			goto ZL1;
		}

		if (!fsm_addedge_epsilon(fsm, (*ZIy), (ZIx))) {
			goto ZL1;
		}

		/* isolation guard */
		/* TODO: centralise */
		{
			struct fsm_state *z;

			z = fsm_addstate(fsm);
			if (z == NULL) {
				goto ZL1;
			}

			if (!fsm_addedge_epsilon(fsm, (*ZIy), z)) {
				goto ZL1;
			}

			(*ZIy) = z;
		}
	
#line 363 "src/libre/form/glob/parser.c"
			}
			/* END OF ACTION: count-0-or-many */
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
p_list_Hof_Hatoms_C_Cany(fsm fsm, flags flags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case (TOK_QMARK):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: add-any */
		{
#line 428 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		if (!fsm_addedge_any(fsm, (ZIx), (ZIy))) {
			goto ZL1;
		}
	
#line 404 "src/libre/form/glob/parser.c"
		}
		/* END OF ACTION: add-any */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Hatoms_C_Cwildcard(fsm fsm, flags flags, lex_state lex_state, act_state act_state, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case (TOK_STAR):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: add-any */
		{
#line 428 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		if (!fsm_addedge_any(fsm, (ZIx), (ZIy))) {
			goto ZL1;
		}
	
#line 439 "src/libre/form/glob/parser.c"
		}
		/* END OF ACTION: add-any */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

void
p_re__glob(fsm fsm, flags flags, lex_state lex_state, act_state act_state)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_fsm__state ZIx;
		t_fsm__state ZIy;

		/* BEGINNING OF ACTION: make-states */
		{
#line 320 "src/libre/parser.act"

		assert(fsm != NULL);
		/* TODO: assert fsm is empty */

		(ZIx) = fsm_getstart(fsm);
		assert((ZIx) != NULL);

		(ZIy) = fsm_addstate(fsm);
		if ((ZIy) == NULL) {
			goto ZL1;
		}

		fsm_setend(fsm, (ZIy), 1);
	
#line 476 "src/libre/form/glob/parser.c"
		}
		/* END OF ACTION: make-states */
		/* BEGINNING OF INLINE: 66 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_QMARK): case (TOK_STAR): case (TOK_CHAR):
				{
					p_list_Hof_Hatoms (fsm, flags, lex_state, act_state, ZIx, ZIy);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL3;
					}
				}
				break;
			default:
				{
					/* BEGINNING OF ACTION: add-epsilon */
					{
#line 400 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIy))) {
			goto ZL3;
		}
	
#line 501 "src/libre/form/glob/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			}
			goto ZL2;
		ZL3:;
			{
				/* BEGINNING OF ACTION: err-expected-atoms */
				{
#line 566 "src/libre/parser.act"

		act_state->e = RE_EXATOMS;
	
#line 516 "src/libre/form/glob/parser.c"
				}
				/* END OF ACTION: err-expected-atoms */
			}
		ZL2:;
		}
		/* END OF INLINE: 66 */
		/* BEGINNING OF INLINE: 67 */
		{
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_EOF):
					break;
				default:
					goto ZL5;
				}
				ADVANCE_LEXER;
			}
			goto ZL4;
		ZL5:;
			{
				/* BEGINNING OF ACTION: err-expected-eof */
				{
#line 578 "src/libre/parser.act"

		act_state->e = RE_EXEOF;
	
#line 543 "src/libre/form/glob/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL4:;
		}
		/* END OF INLINE: 67 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

/* BEGINNING OF TRAILER */

#line 694 "src/libre/parser.act"


	static int
	lgetc(struct LX_STATE *lx)
	{
		const struct lex_state *lex_state;

		assert(lx != NULL);
		assert(lx->opaque != NULL);

		lex_state = lx->opaque;

		assert(lex_state->f != NULL);

		return lex_state->f(lex_state->opaque);
	}

	static int
	parse(int (*f)(void *opaque), void *opaque,
		void (*entry)(struct fsm *, flags, lex_state, act_state),
		enum re_flags flags, struct fsm *new, struct re_err *err)
	{
		struct act_state  act_state_s;
		struct act_state *act_state;
		struct lex_state  lex_state_s;
		struct lex_state *lex_state;

		struct LX_STATE *lx;

		assert(f != NULL);
		assert(entry != NULL);
		assert(new != NULL);

		lex_state    = &lex_state_s;
		lex_state->p = lex_state->a;

		lx = &lex_state->lx;

		LX_INIT(lx);

		lx->lgetc  = lgetc;
		lx->opaque = lex_state;

		lex_state->f       = f;
		lex_state->opaque  = opaque;

		lex_state->buf.a   = NULL;
		lex_state->buf.len = 0;

		/* XXX: unneccessary since we're lexing from a string */
		lx->buf   = &lex_state->buf;
		lx->push  = CAT(LX_PREFIX, _dynpush);
		lx->pop   = CAT(LX_PREFIX, _dynpop);
		lx->clear = CAT(LX_PREFIX, _dynclear);
		lx->free  = CAT(LX_PREFIX, _dynfree);

	/* XXX */
	lx->free = NULL;

		/* This is a workaround for ADVANCE_LEXER assuming a pointer */
		act_state = &act_state_s;

		act_state->e = RE_ESUCCESS;

		ADVANCE_LEXER;
		entry(new, flags, lex_state, act_state);

		if (act_state->e != RE_ESUCCESS) {
			/* TODO: free internals allocated during parsing (are there any?) */
			goto error;
		}

		return 0;

	error:

		if (err != NULL) {
			err->e    = act_state->e;
			err->byte = lx->start.byte;
		}

		return -1;
	}

	struct fsm *
	FORM_COMP(int (*f)(void *opaque), void *opaque,
		enum re_flags flags, struct re_err *err)
	{
		struct fsm *new;

		assert(f != NULL);

		new = re_new_empty();
		if (new == NULL) {
			goto error;
		}

		if (-1 == parse(f, opaque, FORM_ENTRY, flags, new, err)) {
			fsm_free(new);
			return NULL;
		}

		return new;

	error:

		if (err != NULL) {
			err->e    = RE_EERRNO;
			err->byte = 0;
		}

		return NULL;
	}

#line 674 "src/libre/form/glob/parser.c"

/* END OF FILE */
