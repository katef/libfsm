/*
 * Automatically generated from the files:
 *	src/libre/dialect/like/parser.sid
 * and
 *	src/libre/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 137 "src/libre/parser.act"


	#include <assert.h>
	#include <limits.h>
	#include <string.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <errno.h>
	#include <ctype.h>

	#include "libfsm/internal.h" /* XXX */

	#include <fsm/fsm.h>
	#include <fsm/bool.h>
	#include <fsm/pred.h>

	#include <re/re.h>

	#ifndef DIALECT
	#error DIALECT required
	#endif

	#define PASTE(a, b) a ## b
	#define CAT(a, b)   PASTE(a, b)

	#define LX_PREFIX CAT(lx_, DIALECT)

	#define LX_TOKEN   CAT(LX_PREFIX, _token)
	#define LX_STATE   CAT(LX_PREFIX, _lx)
	#define LX_NEXT    CAT(LX_PREFIX, _next)
	#define LX_INIT    CAT(LX_PREFIX, _init)

	#define DIALECT_COMP  CAT(comp_, DIALECT)

	/* XXX: get rid of this; use same %entry% for all grammars */
	#define DIALECT_ENTRY CAT(p_re__, DIALECT)

	#define TOK_CLASS__alnum  TOK_CLASS_ALNUM
	#define TOK_CLASS__alpha  TOK_CLASS_ALPHA
	#define TOK_CLASS__any    TOK_CLASS_ANY
	#define TOK_CLASS__ascii  TOK_CLASS_ASCII
	#define TOK_CLASS__blank  TOK_CLASS_BLANK
	#define TOK_CLASS__cntrl  TOK_CLASS_CNTRL
	#define TOK_CLASS__digit  TOK_CLASS_DIGIT
	#define TOK_CLASS__graph  TOK_CLASS_GRAPH
	#define TOK_CLASS__lower  TOK_CLASS_LOWER
	#define TOK_CLASS__print  TOK_CLASS_PRINT
	#define TOK_CLASS__punct  TOK_CLASS_PUNCT
	#define TOK_CLASS__space  TOK_CLASS_SPACE
	#define TOK_CLASS__spchr  TOK_CLASS_SPCHR
	#define TOK_CLASS__upper  TOK_CLASS_UPPER
	#define TOK_CLASS__word   TOK_CLASS_WORD
	#define TOK_CLASS__xdigit TOK_CLASS_XDIGIT

	#include "parser.h"
	#include "lexer.h"

	#include "../comp.h"
	#include "../../class.h"

	struct grp {
		struct fsm *set;
		struct fsm *dup;
	};

	struct flags {
		enum re_flags flags;
		struct flags *parent;
	};

	typedef char     t_char;
	typedef const char * t_class;
	typedef unsigned t_unsigned;
	typedef unsigned t_pred; /* TODO */

	typedef struct lx_pos t_pos;
	typedef struct fsm * t_fsm;
	typedef enum re_flags t_re__flags;
	typedef struct grp t_grp;

	struct act_state {
		enum LX_TOKEN lex_tok;
		enum LX_TOKEN lex_tok_save;
		int overlap; /* permit overlap in groups */

		/*
		 * Lexical position stored for syntax errors.
		 */
		struct re_pos synstart;
		struct re_pos synend;

		/*
		 * Lexical positions stored for errors which describe multiple tokens.
		 * We're able to store these without needing a stack, because these are
		 * non-recursive productions.
		 */
		struct re_pos groupstart; struct re_pos groupend;
		struct re_pos rangestart; struct re_pos rangeend;
		struct re_pos countstart; struct re_pos countend;
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
	#define ADVANCE_LEXER    do { mark(&act_state->synstart, &lex_state->lx.start); \
	                              mark(&act_state->synend,   &lex_state->lx.end);   \
	                              act_state->lex_tok = LX_NEXT(&lex_state->lx); } while (0)
	#define SAVE_LEXER(tok)  do { act_state->lex_tok_save = act_state->lex_tok; \
	                              act_state->lex_tok = tok;                     } while (0)
	#define RESTORE_LEXER    do { act_state->lex_tok = act_state->lex_tok_save; } while (0)

	static void
	mark(struct re_pos *r, const struct lx_pos *pos)
	{
		assert(r != NULL);
		assert(pos != NULL);

		r->byte = pos->byte;
	}

	/* TODO: centralise perhaps */
	static void
	snprintdots(char *s, size_t sz, const char *msg)
	{
		size_t n;

		assert(s != NULL);
		assert(sz > 3);
		assert(msg != NULL);

		n = sprintf(s, "%.*s", (int) sz - 3 - 1, msg);
		if (n == sz - 3 - 1) {
			strcpy(s + sz, "...");
		}
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

	static int
	escputc(int c, FILE *f)
	{
		char s[5];

		return fputs(escchar(s, sizeof s, c), f);
	}

	static int
	addedge_literal(struct fsm *fsm, enum re_flags flags,
		struct fsm_state *from, struct fsm_state *to, char c)
	{
		assert(fsm != NULL);
		assert(from != NULL);
		assert(to != NULL);

		if (flags & RE_ICASE) {
			if (!fsm_addedge_literal(fsm, from, to, tolower((unsigned char) c))) {
				return 0;
			}

			if (!fsm_addedge_literal(fsm, from, to, toupper((unsigned char) c))) {
				return 0;
			}
		} else {
			if (!fsm_addedge_literal(fsm, from, to, c)) {
				return 0;
			}
		}

		return 1;
	}

	static int
	group_add(struct grp *g, enum re_flags flags, char c)
	{
		const struct fsm_state *p;
		struct fsm_state *start, *end;
		struct fsm *fsm;
		char a[2];
		char *s = a;

		assert(g != NULL);

		a[0] = c;
		a[1] = '\0';

		errno = 0;
		p = fsm_exec(g->set, fsm_sgetc, &s);
		if (p == NULL && errno != 0) {
			return -1;
		}

		if (p == NULL) {
			fsm = g->set;
		} else {
			fsm = g->dup;
		}

		start = fsm_getstart(fsm);
		assert(start != NULL);

		end = fsm_addstate(fsm);
		if (end == NULL) {
			return -1;
		}

		fsm_setend(fsm, end, 1);

		if (!addedge_literal(fsm, flags, start, end, c)) {
			return -1;
		}

		return 0;
	}

	static struct fsm *
	fsm_new_blank(const struct fsm_options *opt)
	{
		struct fsm *new;
		struct fsm_state *start;

		new = fsm_new(opt);
		if (new == NULL) {
			return NULL;
		}

		start = fsm_addstate(new);
		if (start == NULL) {
			goto error;
		}

		fsm_setstart(new, start);

		return new;

	error:

		fsm_free(new);

		return NULL;
	}

	/* TODO: centralise as fsm_unionxy() perhaps */
	static int
	fsm_unionxy(struct fsm *a, struct fsm *b, struct fsm_state *x, struct fsm_state *y)
	{
		struct fsm_state *sa, *sb;
		struct fsm_state *end;

		assert(a != NULL);
		assert(b != NULL);
		assert(x != NULL);
		assert(y != NULL);

		sa = fsm_getstart(a);
		sb = fsm_getstart(b);

		end = fsm_collate(b, fsm_isend);
		if (end == NULL) {
			return 0;
		}

		if (!fsm_merge(a, b)) {
			return 0;
		}

		fsm_setstart(a, sa);

		if (!fsm_addedge_epsilon(a, x, sb)) {
			return 0;
		}

		fsm_setend(a, end, 0);

		if (!fsm_addedge_epsilon(a, end, y)) {
			return 0;
		}

		return 1;
	}

	static struct fsm *
	fsm_new_any(const struct fsm_options *opt)
	{
		struct fsm_state *a, *b;
		struct fsm *new;

		assert(opt != NULL);

		new = fsm_new(opt);
		if (new == NULL) {
			return NULL;
		}

		a = fsm_addstate(new);
		if (a == NULL) {
			goto error;
		}

		b = fsm_addstate(new);
		if (b == NULL) {
			goto error;
		}

		/* TODO: provide as a class. for utf8, this would be /./ */
		if (!fsm_addedge_any(new, a, b)) {
			goto error;
		}

		fsm_setstart(new, a);
		fsm_setend(new, b, 1);

		return new;

	error:

		fsm_free(new);

		return NULL;
	}

	/* XXX: to go when dups show all spellings for group overlap */
	static const struct fsm_state *
	fsm_any(const struct fsm *fsm,
		int (*predicate)(const struct fsm *, const struct fsm_state *))
	{
		const struct fsm_state *s;

		assert(fsm != NULL);
		assert(predicate != NULL);

		for (s = fsm->sl; s != NULL; s = s->next) {
			if (!predicate(fsm, s)) {
				return s;
			}
		}

		return NULL;
	}

#line 407 "src/libre/dialect/like/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Hatoms_C_Cliteral(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_list_Hof_Hatoms(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_list_Hof_Hatoms_C_Catom(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state *);
static void p_list_Hof_Hatoms_C_Cmany(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
static void p_list_Hof_Hatoms_C_Cany(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);
extern void p_re__like(fsm, flags, lex_state, act_state, err, t_fsm__state, t_fsm__state);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Hatoms_C_Cliteral(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZIc;
		t_pos ZI94;
		t_pos ZI95;

		switch (CURRENT_TERMINAL) {
		case (TOK_CHAR):
			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 557 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI94 = lex_state->lx.start;
		ZI95   = lex_state->lx.end;

		ZIc = lex_state->buf.a[0];
	
#line 453 "src/libre/dialect/like/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: add-literal */
		{
#line 831 "src/libre/parser.act"

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		/* TODO: check c is legal? */

		if (!addedge_literal(fsm, flags->flags, (ZIx), (ZIy), (ZIc))) {
			goto ZL1;
		}
	
#line 474 "src/libre/dialect/like/parser.c"
		}
		/* END OF ACTION: add-literal */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Hatoms(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_list_Hof_Hatoms:;
	{
		t_fsm__state ZIz;

		/* BEGINNING OF ACTION: add-concat */
		{
#line 807 "src/libre/parser.act"

		(ZIz) = fsm_addstate(fsm);
		if ((ZIz) == NULL) {
			goto ZL1;
		}
	
#line 503 "src/libre/dialect/like/parser.c"
		}
		/* END OF ACTION: add-concat */
		p_list_Hof_Hatoms_C_Catom (fsm, flags, lex_state, act_state, err, ZIx, &ZIz);
		/* BEGINNING OF INLINE: 103 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY): case (TOK_MANY): case (TOK_CHAR):
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
#line 814 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIz), (ZIy))) {
			goto ZL1;
		}
	
#line 528 "src/libre/dialect/like/parser.c"
					}
					/* END OF ACTION: add-epsilon */
				}
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 103 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Hatoms_C_Catom(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state *ZIy)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_ANY):
		{
			p_list_Hof_Hatoms_C_Cany (fsm, flags, lex_state, act_state, err, ZIx, *ZIy);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: count-1 */
			{
#line 980 "src/libre/parser.act"

		(void) (ZIx);
		(void) (*ZIy);
	
#line 564 "src/libre/dialect/like/parser.c"
			}
			/* END OF ACTION: count-1 */
		}
		break;
	case (TOK_CHAR):
		{
			p_list_Hof_Hatoms_C_Cliteral (fsm, flags, lex_state, act_state, err, ZIx, *ZIy);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: count-1 */
			{
#line 980 "src/libre/parser.act"

		(void) (ZIx);
		(void) (*ZIy);
	
#line 583 "src/libre/dialect/like/parser.c"
			}
			/* END OF ACTION: count-1 */
		}
		break;
	case (TOK_MANY):
		{
			p_list_Hof_Hatoms_C_Cmany (fsm, flags, lex_state, act_state, err, ZIx, *ZIy);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: count-0-or-many */
			{
#line 930 "src/libre/parser.act"

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
	
#line 624 "src/libre/dialect/like/parser.c"
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
p_list_Hof_Hatoms_C_Cmany(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case (TOK_MANY):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: add-any */
		{
#line 844 "src/libre/parser.act"

		struct fsm *any;

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		any = class_any_fsm(fsm->opt);
		if (any == NULL) {
			err->e = RE_EERRNO;
			goto ZL1;
		}

		if (!fsm_unionxy(fsm, any, (ZIx), (ZIy))) {
			err->e = RE_EERRNO;
			goto ZL1;
		}
	
#line 674 "src/libre/dialect/like/parser.c"
		}
		/* END OF ACTION: add-any */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_list_Hof_Hatoms_C_Cany(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case (TOK_ANY):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: add-any */
		{
#line 844 "src/libre/parser.act"

		struct fsm *any;

		assert((ZIx) != NULL);
		assert((ZIy) != NULL);

		any = class_any_fsm(fsm->opt);
		if (any == NULL) {
			err->e = RE_EERRNO;
			goto ZL1;
		}

		if (!fsm_unionxy(fsm, any, (ZIx), (ZIy))) {
			err->e = RE_EERRNO;
			goto ZL1;
		}
	
#line 718 "src/libre/dialect/like/parser.c"
		}
		/* END OF ACTION: add-any */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

void
p_re__like(fsm fsm, flags flags, lex_state lex_state, act_state act_state, err err, t_fsm__state ZIx, t_fsm__state ZIy)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 105 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY): case (TOK_MANY): case (TOK_CHAR):
				{
					p_list_Hof_Hatoms (fsm, flags, lex_state, act_state, err, ZIx, ZIy);
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
#line 814 "src/libre/parser.act"

		if (!fsm_addedge_epsilon(fsm, (ZIx), (ZIy))) {
			goto ZL3;
		}
	
#line 757 "src/libre/dialect/like/parser.c"
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
#line 1029 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXATOMS;
		}
		goto ZL1;
	
#line 775 "src/libre/dialect/like/parser.c"
				}
				/* END OF ACTION: err-expected-atoms */
			}
		ZL2:;
		}
		/* END OF INLINE: 105 */
		/* BEGINNING OF INLINE: 106 */
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
#line 1078 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXEOF;
		}
		goto ZL1;
	
#line 805 "src/libre/dialect/like/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL4:;
		}
		/* END OF INLINE: 106 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

/* BEGINNING OF TRAILER */

#line 1268 "src/libre/parser.act"


	static int
	lgetc(struct LX_STATE *lx)
	{
		struct lex_state *lex_state;

		assert(lx != NULL);
		assert(lx->opaque != NULL);

		lex_state = lx->opaque;

		assert(lex_state->f != NULL);

		return lex_state->f(lex_state->opaque);
	}

	static int
	parse(int (*f)(void *opaque), void *opaque,
		void (*entry)(struct fsm *, flags, lex_state, act_state, err,
			struct fsm_state *, struct fsm_state *),
		struct flags *flags, int overlap,
		struct fsm *new, struct re_err *err,
		struct fsm_state *x, struct fsm_state *y)
	{
		struct act_state  act_state_s;
		struct act_state *act_state;
		struct lex_state  lex_state_s;
		struct lex_state *lex_state;
		struct re_err dummy;

		struct LX_STATE *lx;

		assert(f != NULL);
		assert(entry != NULL);

		if (err == NULL) {
			err = &dummy;
		}

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
		/* (except for pushing "[" and "]" around ::group-$dialect) */
		lx->buf   = &lex_state->buf;
		lx->push  = CAT(LX_PREFIX, _dynpush);
		lx->clear = CAT(LX_PREFIX, _dynclear);
		lx->free  = CAT(LX_PREFIX, _dynfree);

		/* This is a workaround for ADVANCE_LEXER assuming a pointer */
		act_state = &act_state_s;

		act_state->overlap = overlap;

		err->e = RE_ESUCCESS;

		ADVANCE_LEXER;
		entry(new, flags, lex_state, act_state, err, x, y);

		lx->free(lx);

		if (err->e != RE_ESUCCESS) {
			/* TODO: free internals allocated during parsing (are there any?) */
			goto error;
		}

		return 0;

	error:

		/*
		 * Some errors describe multiple tokens; for these, the start and end
		 * positions belong to potentially different tokens, and therefore need
		 * to be stored statefully (in act_state). These are all from
		 * non-recursive productions by design, and so a stack isn't needed.
		 *
		 * Lexical errors describe a problem with a single token; for these,
		 * the start and end positions belong to that token.
		 *
		 * Syntax errors occur at the first point the order of tokens is known
		 * to be incorrect, rather than describing a span of bytes. For these,
		 * the start of the next token is most relevant.
		 */

		switch (err->e) {
		case RE_EOVERLAP:  err->start = act_state->groupstart; err->end = act_state->groupend; break;
		case RE_ENEGRANGE: err->start = act_state->rangestart; err->end = act_state->rangeend; break;
		case RE_ENEGCOUNT: err->start = act_state->countstart; err->end = act_state->countend; break;

		case RE_EHEXRANGE:
		case RE_EOCTRANGE:
		case RE_ECOUNTRANGE:
			/*
			 * Lexical errors: These are always generated for the current token,
			 * so lx->start/end here is correct because ADVANCE_LEXER has
			 * not been called.
			 */
			mark(&err->start, &lx->start);
			mark(&err->end,   &lx->end);
			break;

		default:
			/*
			 * Due to LL(1) lookahead, lx->start/end is the next token.
			 * This is approximately correct as the position of an error,
			 * but to be exactly correct, we store the pos for the previous token.
			 * This is more visible when whitespace exists.
			 */
			err->start = act_state->synstart;
			err->end   = act_state->synstart; /* single point */
			break;
		}

		return -1;
	}

	struct fsm *
	DIALECT_COMP(int (*f)(void *opaque), void *opaque,
		const struct fsm_options *opt,
		enum re_flags flags, int overlap,
		struct re_err *err)
	{
		struct fsm *new;
		struct fsm_state *x, *y;
		struct flags top, *fl = &top;

		top.flags = flags;

		assert(f != NULL);

		new = fsm_new_blank(opt);
		if (new == NULL) {
			return NULL;
		}

		x = fsm_getstart(new);
		assert(x != NULL);

		y = fsm_addstate(new);
		if (y == NULL) {
			goto error;
		}

		fsm_setend(new, y, 1);

		if (-1 == parse(f, opaque, DIALECT_ENTRY, fl, overlap, new, err, x, y)) {
			goto error;
		}

		return new;

	error:

		fsm_free(new);

		return NULL;
	}

#line 994 "src/libre/dialect/like/parser.c"

/* END OF FILE */
