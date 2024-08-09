/*
 * Automatically generated from the files:
 *	src/libre/dialect/native/parser.sid
 * and
 *	src/libre/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 275 "src/libre/parser.act"


	#include <assert.h>
	#include <limits.h>
	#include <string.h>
	#include <stdint.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <errno.h>
	#include <ctype.h>

	#include <re/re.h>
	#include <adt/common.h>

	#include "libre/class.h"
	#include "libre/class_lookup.h"
	#include "libre/ast.h"

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

	#define DIALECT_PARSE  CAT(parse_re_, DIALECT)
	#define DIALECT_CLASS  CAT(re_class_, DIALECT)

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

	#define TOK_CLASS__nspace  TOK_CLASS_NSPACE
	#define TOK_CLASS__ndigit  TOK_CLASS_NDIGIT

	#include "parser.h"
	#include "lexer.h"

	#include "../comp.h"
	#include "../../class.h"

	typedef char     t_char;
	typedef unsigned t_unsigned;
	typedef unsigned t_pred; /* TODO */

	typedef struct lx_pos t_pos;
	typedef enum re_flags t_re__flags;
	typedef const struct class * t_ast__class__id;
	typedef struct ast_count t_ast__count;
	typedef struct ast_endpoint t_endpoint;
	typedef unsigned t_group__id;

	struct act_state {
		struct ast_expr_pool **poolp;

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

		/*
		 * Numbering for capturing groups. By convention these start from 1,
		 * and 0 represents the entire matching text.
		 */
		unsigned group_id;
	};

	struct lex_state {
		struct LX_STATE lx;
		struct lx_dynbuf buf; /* XXX: unneccessary since we're lexing from a string */

		re_getchar_fun *f;
		void *opaque;

		/* TODO: use lx's generated conveniences for the pattern buffer */
		char a[512];
		char *p;
	};

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
			strcpy(s + n, "...");
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

	static void
	advance_lexer(struct lex_state *lex_state, struct act_state *act_state)
	{
		mark(&act_state->synstart, &lex_state->lx.start);
		mark(&act_state->synend,   &lex_state->lx.end);
		act_state->lex_tok = LX_NEXT(&lex_state->lx);
	}

	static enum LX_TOKEN
	current_token(struct lex_state *lex_state, struct act_state *act_state,
		enum re_flags *flags)
	{
#ifndef PCRE_DIALECT
		(void) lex_state;
		(void) flags;
		return act_state->lex_tok;
#else
		if ((*flags & RE_EXTENDED) == 0) {
			switch (act_state->lex_tok) {
			case TOK_WHITESPACE:
			case TOK_NEWLINE:
			case TOK_MAYBE_COMMENT:
				return TOK_CHAR;

			default:
				return act_state->lex_tok;
			}
		} 

		int extended_mode_comment = 0;

	restart:

		if (extended_mode_comment) {
			switch (act_state->lex_tok) {
			case TOK_EOF:
			case TOK_ERROR:
			case TOK_UNKNOWN:
				/* don't eat EOF or errors */
				return act_state->lex_tok;

			case TOK_NEWLINE:
				extended_mode_comment = 0;
				/* fall through */
			default:
				advance_lexer(lex_state, act_state);
				goto restart;
			}
		} else {
			switch (act_state->lex_tok) {
			case TOK_MAYBE_COMMENT:
				extended_mode_comment = 1;
				/* fall through */
			case TOK_WHITESPACE:
			case TOK_NEWLINE:
				advance_lexer(lex_state, act_state);
				goto restart;

			default:
				return act_state->lex_tok;
			}
		}
#endif
	}

	#define ADVANCE_LEXER    (advance_lexer(lex_state, act_state))
	#define CURRENT_TERMINAL (current_token(lex_state, act_state, flags))
	#define ERROR_TERMINAL   (TOK_ERROR)

	#define SAVE_LEXER(tok)  do { act_state->lex_tok_save = act_state->lex_tok; \
	                              act_state->lex_tok = tok;                     } while (0)
	#define RESTORE_LEXER    do { act_state->lex_tok = act_state->lex_tok_save; } while (0)

#line 273 "src/libre/dialect/native/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_C_Crange_Hendpoint_Hliteral(flags, lex_state, act_state, err, t_endpoint *, t_pos *, t_pos *);
static void p_265(flags, lex_state, act_state, err, t_pos *, t_unsigned *, t_ast__count *);
static void p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms(flags, lex_state, act_state, err, t_ast__expr);
static void p_154(flags, lex_state, act_state, err);
static void p_expr_C_Clist_Hof_Hpieces(flags, lex_state, act_state, err, t_ast__expr);
static void p_expr_C_Cliteral(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Ccharacter_Hclass_C_Cclass_Hterm(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_C_Crange_Hendpoint_Hclass(flags, lex_state, act_state, err, t_endpoint *, t_pos *, t_pos *);
static void p_expr_C_Ccharacter_Hclass(flags, lex_state, act_state, err, t_ast__expr *);
static void p_180(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_Hend(flags, lex_state, act_state, err, t_endpoint *, t_pos *);
static void p_expr_C_Cpiece(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr(flags, lex_state, act_state, err, t_ast__expr *);
static void p_195(flags, lex_state, act_state, err, t_pos *, t_char *, t_ast__expr *);
extern void p_re__native(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint(flags, lex_state, act_state, err, t_endpoint *, t_pos *, t_pos *);
static void p_expr_C_Clist_Hof_Halts(flags, lex_state, act_state, err, t_ast__expr);
static void p_expr_C_Cpiece_C_Ccount(flags, lex_state, act_state, err, t_ast__count *);
static void p_expr_C_Cpiece_C_Catom(flags, lex_state, act_state, err, t_ast__expr *);
static void p_246(flags, lex_state, act_state, err, t_ast__class__id *, t_pos *, t_ast__expr *);
static void p_expr_C_Calt(flags, lex_state, act_state, err, t_ast__expr *);
static void p_250(flags, lex_state, act_state, err, t_char *, t_pos *, t_ast__expr *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_C_Crange_Hendpoint_Hliteral(flags flags, lex_state lex_state, act_state act_state, err err, t_endpoint *ZOr, t_pos *ZOstart, t_pos *ZOend)
{
	t_endpoint ZIr;
	t_pos ZIstart;
	t_pos ZIend;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZIc;

		/* BEGINNING OF INLINE: 141 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 579 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		(void) ZIstart;
		(void) ZIend;

		ZIc = lex_state->buf.a[0];
	
#line 343 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_ESC):
				{
					/* BEGINNING OF EXTRACT: ESC */
					{
#line 391 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZIc = lex_state->buf.a[1];

		switch (ZIc) {
		case 'a': ZIc = '\a'; break;
		case 'b': ZIc = '\b'; break;
		case 'e': ZIc = '\033'; break;
		case 'f': ZIc = '\f'; break;
		case 'n': ZIc = '\n'; break;
		case 'r': ZIc = '\r'; break;
		case 't': ZIc = '\t'; break;
		case 'v': ZIc = '\v'; break;
		default:             break;
		}

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		(void) ZIstart;
		(void) ZIend;
	
#line 379 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: ESC */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_HEX):
				{
					/* BEGINNING OF EXTRACT: HEX */
					{
#line 535 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 2);  /* pcre allows \x without a suffix */

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		(void) ZIstart;
		(void) ZIend;

		errno = 0;

		s = lex_state->buf.a + 2;

		if (*s == '{') {
			s++;
			brace = 1;
		}

		if (*s == '\0') {
			u = 0;
			e = s;
		} else {
			u = strtoul(s, &e, 16);
		}

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EHEXRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if (brace && *e == '}') {
			e++;
		}

		if ((u == ULONG_MAX && errno != 0) || (*e != '\0')) {
			err->e = RE_EXESC;
			goto ZL1;
		}

		ZIc = (char) (unsigned char) u;
	
#line 437 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: HEX */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_OCT):
				{
					/* BEGINNING OF EXTRACT: OCT */
					{
#line 492 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		(void) ZIstart;
		(void) ZIend;

		errno = 0;

		s = lex_state->buf.a + 1;

		if (s[0] == 'o' && s[1] == '{') {
			s += 2;
			brace = 1;
		}

		u = strtoul(s, &e, 8);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EOCTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if (brace && *e == '}') {
			e++;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXESC;
			goto ZL1;
		}

		ZIc = (char) (unsigned char) u;
	
#line 490 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: OCT */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 141 */
		/* BEGINNING OF ACTION: ast-range-endpoint-literal */
		{
#line 840 "src/libre/parser.act"

		(ZIr).type = AST_ENDPOINT_LITERAL;
		(ZIr).u.literal.c = (unsigned char) (ZIc);
	
#line 508 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: ast-range-endpoint-literal */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOr = ZIr;
	*ZOstart = ZIstart;
	*ZOend = ZIend;
}

static void
p_265(flags flags, lex_state lex_state, act_state act_state, err err, t_pos *ZI263, t_unsigned *ZIm, t_ast__count *ZOc)
{
	t_ast__count ZIc;

	switch (CURRENT_TERMINAL) {
	case (TOK_CLOSECOUNT):
		{
			t_pos ZI216;
			t_pos ZIend;

			/* BEGINNING OF EXTRACT: CLOSECOUNT */
			{
#line 379 "src/libre/parser.act"

		ZI216 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		(void) ZI216;
		(void) ZIend;
	
#line 543 "src/libre/dialect/native/parser.c"
			}
			/* END OF EXTRACT: CLOSECOUNT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: mark-count */
			{
#line 778 "src/libre/parser.act"

		mark(&act_state->countstart, &(*ZI263));
		mark(&act_state->countend,   &(ZIend));
	
#line 554 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: mark-count */
			/* BEGINNING OF ACTION: count-range */
			{
#line 825 "src/libre/parser.act"

		if ((*ZIm) < (*ZIm)) {
			err->e = RE_ENEGCOUNT;
			err->m = (*ZIm);
			err->n = (*ZIm);

			mark(&act_state->countstart, &(*ZI263));
			mark(&act_state->countend,   &(ZIend));

			goto ZL1;
		}

		(ZIc) = ast_make_count((*ZIm), (*ZIm));
	
#line 574 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: count-range */
		}
		break;
	case (TOK_SEP):
		{
			t_unsigned ZIn;
			t_pos ZI219;
			t_pos ZIend;

			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (TOK_COUNT):
				/* BEGINNING OF EXTRACT: COUNT */
				{
#line 636 "src/libre/parser.act"

		unsigned long u;
		char *e;

		u = strtoul(lex_state->buf.a, &e, 10);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UINT_MAX) {
			err->e = RE_ECOUNTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXCOUNT;
			goto ZL1;
		}

		ZIn = (unsigned int) u;
	
#line 610 "src/libre/dialect/native/parser.c"
				}
				/* END OF EXTRACT: COUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSECOUNT):
				/* BEGINNING OF EXTRACT: CLOSECOUNT */
				{
#line 379 "src/libre/parser.act"

		ZI219 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		(void) ZI219;
		(void) ZIend;
	
#line 630 "src/libre/dialect/native/parser.c"
				}
				/* END OF EXTRACT: CLOSECOUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: mark-count */
			{
#line 778 "src/libre/parser.act"

		mark(&act_state->countstart, &(*ZI263));
		mark(&act_state->countend,   &(ZIend));
	
#line 645 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: mark-count */
			/* BEGINNING OF ACTION: count-range */
			{
#line 825 "src/libre/parser.act"

		if ((ZIn) < (*ZIm)) {
			err->e = RE_ENEGCOUNT;
			err->m = (*ZIm);
			err->n = (ZIn);

			mark(&act_state->countstart, &(*ZI263));
			mark(&act_state->countend,   &(ZIend));

			goto ZL1;
		}

		(ZIc) = ast_make_count((*ZIm), (ZIn));
	
#line 665 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: count-range */
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
	*ZOc = ZIc;
}

static void
p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr ZIclass)
{
ZL2_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms:;
	switch (CURRENT_TERMINAL) {
	case (TOK_NAMED__CLASS): case (TOK_ESC): case (TOK_OCT): case (TOK_HEX):
	case (TOK_CHAR):
		{
			t_ast__expr ZInode;

			p_expr_C_Ccharacter_Hclass_C_Cclass_Hterm (flags, lex_state, act_state, err, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: ast-add-alt */
			{
#line 1047 "src/libre/parser.act"

		if (!ast_add_expr_alt((ZIclass), (ZInode))) {
			goto ZL1;
		}
	
#line 706 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: ast-add-alt */
			/* BEGINNING OF INLINE: expr::character-class::list-of-class-terms */
			goto ZL2_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms;
			/* END OF INLINE: expr::character-class::list-of-class-terms */
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
p_154(flags flags, lex_state lex_state, act_state act_state, err err)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZI155;
		t_pos ZI156;
		t_pos ZI157;

		switch (CURRENT_TERMINAL) {
		case (TOK_RANGE):
			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 309 "src/libre/parser.act"

		ZI155 = '-';
		ZI156 = lex_state->lx.start;
		ZI157   = lex_state->lx.end;

		(void) ZI155;
		(void) ZI156;
		(void) ZI157;
	
#line 750 "src/libre/dialect/native/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-range */
		{
#line 722 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXRANGE;
		}
		goto ZL2;
	
#line 771 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: err-expected-range */
	}
	goto ZL0;
ZL2:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
}

static void
p_expr_C_Clist_Hof_Hpieces(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr ZIcat)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_expr_C_Clist_Hof_Hpieces:;
	{
		t_ast__expr ZIa;

		p_expr_C_Cpiece (flags, lex_state, act_state, err, &ZIa);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
		/* BEGINNING OF ACTION: ast-add-concat */
		{
#line 1041 "src/libre/parser.act"

		if (!ast_add_expr_concat((ZIcat), (ZIa))) {
			goto ZL1;
		}
	
#line 805 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: ast-add-concat */
		/* BEGINNING OF INLINE: 223 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY): case (TOK_START): case (TOK_END): case (TOK_OPENSUB):
			case (TOK_OPENGROUP): case (TOK_OPENGROUPINV): case (TOK_OPENGROUPCB): case (TOK_OPENGROUPINVCB):
			case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
				{
					/* BEGINNING OF INLINE: expr::list-of-pieces */
					goto ZL2_expr_C_Clist_Hof_Hpieces;
					/* END OF INLINE: expr::list-of-pieces */
				}
				/*UNREACHED*/
			default:
				break;
			}
		}
		/* END OF INLINE: 223 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_expr_C_Cliteral(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZIc;

		/* BEGINNING OF INLINE: 109 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					t_pos ZI117;
					t_pos ZI118;

					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 579 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI117 = lex_state->lx.start;
		ZI118   = lex_state->lx.end;

		(void) ZI117;
		(void) ZI118;

		ZIc = lex_state->buf.a[0];
	
#line 866 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_ESC):
				{
					t_pos ZI111;
					t_pos ZI112;

					/* BEGINNING OF EXTRACT: ESC */
					{
#line 391 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZIc = lex_state->buf.a[1];

		switch (ZIc) {
		case 'a': ZIc = '\a'; break;
		case 'b': ZIc = '\b'; break;
		case 'e': ZIc = '\033'; break;
		case 'f': ZIc = '\f'; break;
		case 'n': ZIc = '\n'; break;
		case 'r': ZIc = '\r'; break;
		case 't': ZIc = '\t'; break;
		case 'v': ZIc = '\v'; break;
		default:             break;
		}

		ZI111 = lex_state->lx.start;
		ZI112   = lex_state->lx.end;

		(void) ZI111;
		(void) ZI112;
	
#line 905 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: ESC */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_HEX):
				{
					t_pos ZI115;
					t_pos ZI116;

					/* BEGINNING OF EXTRACT: HEX */
					{
#line 535 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 2);  /* pcre allows \x without a suffix */

		ZI115 = lex_state->lx.start;
		ZI116   = lex_state->lx.end;

		(void) ZI115;
		(void) ZI116;

		errno = 0;

		s = lex_state->buf.a + 2;

		if (*s == '{') {
			s++;
			brace = 1;
		}

		if (*s == '\0') {
			u = 0;
			e = s;
		} else {
			u = strtoul(s, &e, 16);
		}

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EHEXRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if (brace && *e == '}') {
			e++;
		}

		if ((u == ULONG_MAX && errno != 0) || (*e != '\0')) {
			err->e = RE_EXESC;
			goto ZL1;
		}

		ZIc = (char) (unsigned char) u;
	
#line 966 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: HEX */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_OCT):
				{
					t_pos ZI113;
					t_pos ZI114;

					/* BEGINNING OF EXTRACT: OCT */
					{
#line 492 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		ZI113 = lex_state->lx.start;
		ZI114   = lex_state->lx.end;

		(void) ZI113;
		(void) ZI114;

		errno = 0;

		s = lex_state->buf.a + 1;

		if (s[0] == 'o' && s[1] == '{') {
			s += 2;
			brace = 1;
		}

		u = strtoul(s, &e, 8);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EOCTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if (brace && *e == '}') {
			e++;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXESC;
			goto ZL1;
		}

		ZIc = (char) (unsigned char) u;
	
#line 1022 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: OCT */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 109 */
		/* BEGINNING OF ACTION: ast-make-literal */
		{
#line 875 "src/libre/parser.act"

		(ZInode) = ast_make_expr_literal(act_state->poolp, *flags, (ZIc));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1042 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: ast-make-literal */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Ccharacter_Hclass_C_Cclass_Hterm(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_CHAR):
		{
			t_char ZI259;
			t_pos ZI260;
			t_pos ZI261;

			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 579 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI260 = lex_state->lx.start;
		ZI261   = lex_state->lx.end;

		(void) ZI260;
		(void) ZI261;

		ZI259 = lex_state->buf.a[0];
	
#line 1081 "src/libre/dialect/native/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			ADVANCE_LEXER;
			p_250 (flags, lex_state, act_state, err, &ZI259, &ZI260, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_ESC):
		{
			t_char ZI247;
			t_pos ZI248;
			t_pos ZI249;

			/* BEGINNING OF EXTRACT: ESC */
			{
#line 391 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZI247 = lex_state->buf.a[1];

		switch (ZI247) {
		case 'a': ZI247 = '\a'; break;
		case 'b': ZI247 = '\b'; break;
		case 'e': ZI247 = '\033'; break;
		case 'f': ZI247 = '\f'; break;
		case 'n': ZI247 = '\n'; break;
		case 'r': ZI247 = '\r'; break;
		case 't': ZI247 = '\t'; break;
		case 'v': ZI247 = '\v'; break;
		default:             break;
		}

		ZI248 = lex_state->lx.start;
		ZI249   = lex_state->lx.end;

		(void) ZI248;
		(void) ZI249;
	
#line 1126 "src/libre/dialect/native/parser.c"
			}
			/* END OF EXTRACT: ESC */
			ADVANCE_LEXER;
			p_250 (flags, lex_state, act_state, err, &ZI247, &ZI248, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_HEX):
		{
			t_char ZI255;
			t_pos ZI256;
			t_pos ZI257;

			/* BEGINNING OF EXTRACT: HEX */
			{
#line 535 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 2);  /* pcre allows \x without a suffix */

		ZI256 = lex_state->lx.start;
		ZI257   = lex_state->lx.end;

		(void) ZI256;
		(void) ZI257;

		errno = 0;

		s = lex_state->buf.a + 2;

		if (*s == '{') {
			s++;
			brace = 1;
		}

		if (*s == '\0') {
			u = 0;
			e = s;
		} else {
			u = strtoul(s, &e, 16);
		}

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EHEXRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if (brace && *e == '}') {
			e++;
		}

		if ((u == ULONG_MAX && errno != 0) || (*e != '\0')) {
			err->e = RE_EXESC;
			goto ZL1;
		}

		ZI255 = (char) (unsigned char) u;
	
#line 1193 "src/libre/dialect/native/parser.c"
			}
			/* END OF EXTRACT: HEX */
			ADVANCE_LEXER;
			p_250 (flags, lex_state, act_state, err, &ZI255, &ZI256, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_NAMED__CLASS):
		{
			t_ast__class__id ZI243;
			t_pos ZI244;
			t_pos ZI245;

			/* BEGINNING OF EXTRACT: NAMED_CLASS */
			{
#line 648 "src/libre/parser.act"

		ZI243 = DIALECT_CLASS(lex_state->buf.a);
		if (ZI243 == NULL) {
			/* syntax error -- unrecognized class */
			goto ZL1;
		}

		ZI244 = lex_state->lx.start;
		ZI245   = lex_state->lx.end;

		(void) ZI244;
		(void) ZI245;
	
#line 1226 "src/libre/dialect/native/parser.c"
			}
			/* END OF EXTRACT: NAMED_CLASS */
			ADVANCE_LEXER;
			p_246 (flags, lex_state, act_state, err, &ZI243, &ZI244, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_OCT):
		{
			t_char ZI251;
			t_pos ZI252;
			t_pos ZI253;

			/* BEGINNING OF EXTRACT: OCT */
			{
#line 492 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		ZI252 = lex_state->lx.start;
		ZI253   = lex_state->lx.end;

		(void) ZI252;
		(void) ZI253;

		errno = 0;

		s = lex_state->buf.a + 1;

		if (s[0] == 'o' && s[1] == '{') {
			s += 2;
			brace = 1;
		}

		u = strtoul(s, &e, 8);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UCHAR_MAX) {
			err->e = RE_EOCTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if (brace && *e == '}') {
			e++;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXESC;
			goto ZL1;
		}

		ZI251 = (char) (unsigned char) u;
	
#line 1288 "src/libre/dialect/native/parser.c"
			}
			/* END OF EXTRACT: OCT */
			ADVANCE_LEXER;
			p_250 (flags, lex_state, act_state, err, &ZI251, &ZI252, &ZInode);
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
	*ZOnode = ZInode;
}

static void
p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_C_Crange_Hendpoint_Hclass(flags flags, lex_state lex_state, act_state act_state, err err, t_endpoint *ZOr, t_pos *ZOstart, t_pos *ZOend)
{
	t_endpoint ZIr;
	t_pos ZIstart;
	t_pos ZIend;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__class__id ZIid;

		switch (CURRENT_TERMINAL) {
		case (TOK_NAMED__CLASS):
			/* BEGINNING OF EXTRACT: NAMED_CLASS */
			{
#line 648 "src/libre/parser.act"

		ZIid = DIALECT_CLASS(lex_state->buf.a);
		if (ZIid == NULL) {
			/* syntax error -- unrecognized class */
			goto ZL1;
		}

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		(void) ZIstart;
		(void) ZIend;
	
#line 1343 "src/libre/dialect/native/parser.c"
			}
			/* END OF EXTRACT: NAMED_CLASS */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: ast-range-endpoint-class */
		{
#line 845 "src/libre/parser.act"

		(ZIr).type = AST_ENDPOINT_NAMED;
		(ZIr).u.named.class = (ZIid);
	
#line 1358 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: ast-range-endpoint-class */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOr = ZIr;
	*ZOstart = ZIstart;
	*ZOend = ZIend;
}

static void
p_expr_C_Ccharacter_Hclass(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_pos ZIstart;
		t_ast__expr ZItmp;

		/* BEGINNING OF INLINE: 166 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_OPENGROUP):
				{
					t_pos ZI167;

					/* BEGINNING OF EXTRACT: OPENGROUP */
					{
#line 319 "src/libre/parser.act"

		ZIstart = lex_state->lx.start;
		ZI167   = lex_state->lx.end;

		(void) ZIstart;
		(void) ZI167;
	
#line 1401 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: OPENGROUP */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-make-alt */
					{
#line 868 "src/libre/parser.act"

		(ZInode) = ast_make_expr_alt(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1414 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: ast-make-alt */
					ZItmp = ZInode;
					p_180 (flags, lex_state, act_state, err, &ZItmp);
					p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms (flags, lex_state, act_state, err, ZItmp);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_OPENGROUPCB):
				{
					t_pos ZI186;
					t_char ZIcbrak;
					t_ast__expr ZInode1;

					/* BEGINNING OF EXTRACT: OPENGROUPCB */
					{
#line 335 "src/libre/parser.act"

		ZIstart = lex_state->lx.start;
		ZI186   = lex_state->lx.end;

		(void) ZIstart;
		(void) ZI186;
	
#line 1442 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: OPENGROUPCB */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-make-alt */
					{
#line 868 "src/libre/parser.act"

		(ZInode) = ast_make_expr_alt(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1455 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: ast-make-alt */
					ZItmp = ZInode;
					/* BEGINNING OF ACTION: make-literal-cbrak */
					{
#line 886 "src/libre/parser.act"

		(ZIcbrak) = ']';
	
#line 1465 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: make-literal-cbrak */
					p_195 (flags, lex_state, act_state, err, &ZIstart, &ZIcbrak, &ZInode1);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
					/* BEGINNING OF ACTION: ast-add-alt */
					{
#line 1047 "src/libre/parser.act"

		if (!ast_add_expr_alt((ZItmp), (ZInode1))) {
			goto ZL1;
		}
	
#line 1481 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: ast-add-alt */
					p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms (flags, lex_state, act_state, err, ZItmp);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_OPENGROUPINV):
				{
					t_pos ZI178;

					/* BEGINNING OF EXTRACT: OPENGROUPINV */
					{
#line 327 "src/libre/parser.act"

		ZIstart = lex_state->lx.start;
		ZI178   = lex_state->lx.end;

		(void) ZIstart;
		(void) ZI178;
	
#line 1505 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: OPENGROUPINV */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-make-alt */
					{
#line 868 "src/libre/parser.act"

		(ZInode) = ast_make_expr_alt(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1518 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: ast-make-alt */
					ZItmp = ZInode;
					/* BEGINNING OF ACTION: ast-make-invert */
					{
#line 995 "src/libre/parser.act"

		struct ast_expr *any;

		/*
		 * Here we're using an AST_EXPR_SUBTRACT node to implement inversion.
		 *
		 * Inversion is not quite equivalent to fsm_complement, because the
		 * complement of a class FSM (which is structured always left-to-right
		 * with no repetition) will effectively create .* anchors at the start
		 * and end (because the class didn't contain those).
		 *
		 * If those were added to a class beforehand, then fsm_complement()
		 * would suffice here (which would be attractive because it's much
		 * cheaper than constructing two FSM and subtracting them). However
		 * that would entail the assumption of having no transitions back to
		 * the start and end nodes, or introducing guard epsilons.
		 *
		 * I'd like to avoid those guards. And, unfortunately we'd still need
		 * to construct an intermediate FSM for fsm_complement() in any case.
		 * (And less typical, but we do still need AST_EXPR_SUBTRACT for sake
		 * of the SQL 2003 dialect anyway).
		 *
		 * So that idea doesn't satisfy my goal of avoiding intermediate FSM,
		 * and so I'm going with the simpler thing here until we come across
		 * a better idea.
		 */

		any = ast_make_expr_named(act_state->poolp, *flags, &class_any);
		if (any == NULL) {
			goto ZL1;
		}

		(ZInode) = ast_make_expr_subtract(act_state->poolp, *flags, any, (ZInode));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1562 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: ast-make-invert */
					p_180 (flags, lex_state, act_state, err, &ZItmp);
					p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms (flags, lex_state, act_state, err, ZItmp);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_OPENGROUPINVCB):
				{
					t_pos ZI193;
					t_char ZIcbrak;
					t_ast__expr ZInode1;

					/* BEGINNING OF EXTRACT: OPENGROUPINVCB */
					{
#line 343 "src/libre/parser.act"

		ZIstart = lex_state->lx.start;
		ZI193   = lex_state->lx.end;

		(void) ZIstart;
		(void) ZI193;
	
#line 1589 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: OPENGROUPINVCB */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-make-alt */
					{
#line 868 "src/libre/parser.act"

		(ZInode) = ast_make_expr_alt(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1602 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: ast-make-alt */
					ZItmp = ZInode;
					/* BEGINNING OF ACTION: ast-make-invert */
					{
#line 995 "src/libre/parser.act"

		struct ast_expr *any;

		/*
		 * Here we're using an AST_EXPR_SUBTRACT node to implement inversion.
		 *
		 * Inversion is not quite equivalent to fsm_complement, because the
		 * complement of a class FSM (which is structured always left-to-right
		 * with no repetition) will effectively create .* anchors at the start
		 * and end (because the class didn't contain those).
		 *
		 * If those were added to a class beforehand, then fsm_complement()
		 * would suffice here (which would be attractive because it's much
		 * cheaper than constructing two FSM and subtracting them). However
		 * that would entail the assumption of having no transitions back to
		 * the start and end nodes, or introducing guard epsilons.
		 *
		 * I'd like to avoid those guards. And, unfortunately we'd still need
		 * to construct an intermediate FSM for fsm_complement() in any case.
		 * (And less typical, but we do still need AST_EXPR_SUBTRACT for sake
		 * of the SQL 2003 dialect anyway).
		 *
		 * So that idea doesn't satisfy my goal of avoiding intermediate FSM,
		 * and so I'm going with the simpler thing here until we come across
		 * a better idea.
		 */

		any = ast_make_expr_named(act_state->poolp, *flags, &class_any);
		if (any == NULL) {
			goto ZL1;
		}

		(ZInode) = ast_make_expr_subtract(act_state->poolp, *flags, any, (ZInode));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1646 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: ast-make-invert */
					/* BEGINNING OF ACTION: make-literal-cbrak */
					{
#line 886 "src/libre/parser.act"

		(ZIcbrak) = ']';
	
#line 1655 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: make-literal-cbrak */
					p_195 (flags, lex_state, act_state, err, &ZIstart, &ZIcbrak, &ZInode1);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
					/* BEGINNING OF ACTION: ast-add-alt */
					{
#line 1047 "src/libre/parser.act"

		if (!ast_add_expr_alt((ZItmp), (ZInode1))) {
			goto ZL1;
		}
	
#line 1671 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: ast-add-alt */
					p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms (flags, lex_state, act_state, err, ZItmp);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 166 */
		/* BEGINNING OF INLINE: 199 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSEGROUP):
				{
					t_char ZI200;
					t_pos ZI201;
					t_pos ZIend;

					/* BEGINNING OF EXTRACT: CLOSEGROUP */
					{
#line 351 "src/libre/parser.act"

		ZI200 = ']';
		ZI201 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		(void) ZI200;
		(void) ZI201;
		(void) ZIend;
	
#line 1707 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: CLOSEGROUP */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: mark-group */
					{
#line 768 "src/libre/parser.act"

		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));
	
#line 1718 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: mark-group */
				}
				break;
			case (TOK_CLOSEGROUPRANGE):
				{
					t_char ZIcrange;
					t_pos ZI203;
					t_pos ZIend;
					t_ast__expr ZIrange;

					/* BEGINNING OF EXTRACT: CLOSEGROUPRANGE */
					{
#line 361 "src/libre/parser.act"

		ZIcrange = '-';
		ZI203 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		(void) ZIcrange;
		(void) ZI203;
		(void) ZIend;
	
#line 1742 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: CLOSEGROUPRANGE */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-make-literal */
					{
#line 875 "src/libre/parser.act"

		(ZIrange) = ast_make_expr_literal(act_state->poolp, *flags, (ZIcrange));
		if ((ZIrange) == NULL) {
			goto ZL4;
		}
	
#line 1755 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: ast-make-literal */
					/* BEGINNING OF ACTION: ast-add-alt */
					{
#line 1047 "src/libre/parser.act"

		if (!ast_add_expr_alt((ZItmp), (ZIrange))) {
			goto ZL4;
		}
	
#line 1766 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: ast-add-alt */
					/* BEGINNING OF ACTION: mark-group */
					{
#line 768 "src/libre/parser.act"

		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));
	
#line 1776 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: mark-group */
				}
				break;
			default:
				goto ZL4;
			}
			goto ZL3;
		ZL4:;
			{
				/* BEGINNING OF ACTION: err-expected-closegroup */
				{
#line 729 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCLOSEGROUP;
		}
		goto ZL1;
	
#line 1796 "src/libre/dialect/native/parser.c"
				}
				/* END OF ACTION: err-expected-closegroup */
			}
		ZL3:;
		}
		/* END OF INLINE: 199 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_180(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZItmp)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZIc;
			t_pos ZIrstart;
			t_pos ZI181;
			t_ast__expr ZInode1;

			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 309 "src/libre/parser.act"

		ZIc = '-';
		ZIrstart = lex_state->lx.start;
		ZI181   = lex_state->lx.end;

		(void) ZIc;
		(void) ZIrstart;
		(void) ZI181;
	
#line 1835 "src/libre/dialect/native/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			/* BEGINNING OF INLINE: 182 */
			{
				switch (CURRENT_TERMINAL) {
				default:
					{
						/* BEGINNING OF ACTION: ast-make-literal */
						{
#line 875 "src/libre/parser.act"

		(ZInode1) = ast_make_expr_literal(act_state->poolp, *flags, (ZIc));
		if ((ZInode1) == NULL) {
			goto ZL1;
		}
	
#line 1853 "src/libre/dialect/native/parser.c"
						}
						/* END OF ACTION: ast-make-literal */
					}
					break;
				case (TOK_RANGE):
					{
						t_endpoint ZIlower;
						t_char ZI183;
						t_pos ZI184;
						t_pos ZI185;
						t_endpoint ZIupper;
						t_pos ZIend;

						/* BEGINNING OF ACTION: ast-range-endpoint-literal */
						{
#line 840 "src/libre/parser.act"

		(ZIlower).type = AST_ENDPOINT_LITERAL;
		(ZIlower).u.literal.c = (unsigned char) (ZIc);
	
#line 1874 "src/libre/dialect/native/parser.c"
						}
						/* END OF ACTION: ast-range-endpoint-literal */
						/* BEGINNING OF EXTRACT: RANGE */
						{
#line 309 "src/libre/parser.act"

		ZI183 = '-';
		ZI184 = lex_state->lx.start;
		ZI185   = lex_state->lx.end;

		(void) ZI183;
		(void) ZI184;
		(void) ZI185;
	
#line 1889 "src/libre/dialect/native/parser.c"
						}
						/* END OF EXTRACT: RANGE */
						ADVANCE_LEXER;
						p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_Hend (flags, lex_state, act_state, err, &ZIupper, &ZIend);
						if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
							RESTORE_LEXER;
							goto ZL1;
						}
						/* BEGINNING OF ACTION: ast-make-range */
						{
#line 1007 "src/libre/parser.act"

		unsigned char lower, upper;

		if ((ZIlower).type != AST_ENDPOINT_LITERAL ||
			(ZIupper).type != AST_ENDPOINT_LITERAL) {
			err->e = RE_EUNSUPPORTED;
			goto ZL1;
		}

		lower = (ZIlower).u.literal.c;
		upper = (ZIupper).u.literal.c;

		if (lower > upper) {
			char a[5], b[5];
			
			assert(sizeof err->set >= 1 + sizeof a + 1 + sizeof b + 1 + 1);
			
			sprintf(err->set, "%s-%s",
				escchar(a, sizeof a, lower), escchar(b, sizeof b, upper));
			err->e = RE_ENEGRANGE;
			goto ZL1;
		}

		(ZInode1) = ast_make_expr_range(act_state->poolp, *flags, &(ZIlower), &(ZIupper));
		if ((ZInode1) == NULL) {
			goto ZL1;
		}
	
#line 1929 "src/libre/dialect/native/parser.c"
						}
						/* END OF ACTION: ast-make-range */
					}
					break;
				}
			}
			/* END OF INLINE: 182 */
			/* BEGINNING OF ACTION: ast-add-alt */
			{
#line 1047 "src/libre/parser.act"

		if (!ast_add_expr_alt((*ZItmp), (ZInode1))) {
			goto ZL1;
		}
	
#line 1945 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: ast-add-alt */
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
p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_Hend(flags flags, lex_state lex_state, act_state act_state, err err, t_endpoint *ZOr, t_pos *ZOend)
{
	t_endpoint ZIr;
	t_pos ZIend;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 147 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_RANGE):
				{
					t_char ZIc;
					t_pos ZI149;

					/* BEGINNING OF EXTRACT: RANGE */
					{
#line 309 "src/libre/parser.act"

		ZIc = '-';
		ZI149 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		(void) ZIc;
		(void) ZI149;
		(void) ZIend;
	
#line 1991 "src/libre/dialect/native/parser.c"
					}
					/* END OF EXTRACT: RANGE */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-range-endpoint-literal */
					{
#line 840 "src/libre/parser.act"

		(ZIr).type = AST_ENDPOINT_LITERAL;
		(ZIr).u.literal.c = (unsigned char) (ZIc);
	
#line 2002 "src/libre/dialect/native/parser.c"
					}
					/* END OF ACTION: ast-range-endpoint-literal */
				}
				break;
			case (TOK_NAMED__CLASS): case (TOK_ESC): case (TOK_OCT): case (TOK_HEX):
			case (TOK_CHAR):
				{
					t_pos ZI148;

					p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint (flags, lex_state, act_state, err, &ZIr, &ZI148, &ZIend);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 147 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOr = ZIr;
	*ZOend = ZIend;
}

static void
p_expr_C_Cpiece(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__expr ZIe;
		t_ast__count ZIc;

		p_expr_C_Cpiece_C_Catom (flags, lex_state, act_state, err, &ZIe);
		p_expr_C_Cpiece_C_Ccount (flags, lex_state, act_state, err, &ZIc);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
		/* BEGINNING OF ACTION: ast-make-piece */
		{
#line 898 "src/libre/parser.act"

		if ((ZIc).min == 0 && (ZIc).max == 0) {
			(ZInode) = ast_make_expr_empty(act_state->poolp, *flags);
		} else if ((ZIc).min == 1 && (ZIc).max == 1) {
			(ZInode) = (ZIe);
		} else {
			(ZInode) = ast_make_expr_repeat(act_state->poolp, *flags, (ZIe), (ZIc));
		}
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL1;
		}
	
#line 2068 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: ast-make-piece */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_ANY): case (TOK_START): case (TOK_END): case (TOK_OPENSUB):
	case (TOK_OPENGROUP): case (TOK_OPENGROUPINV): case (TOK_OPENGROUPCB): case (TOK_OPENGROUPINVCB):
	case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
		{
			/* BEGINNING OF ACTION: ast-make-alt */
			{
#line 868 "src/libre/parser.act"

		(ZInode) = ast_make_expr_alt(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2099 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: ast-make-alt */
			p_expr_C_Clist_Hof_Halts (flags, lex_state, act_state, err, ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: ast-make-empty */
			{
#line 854 "src/libre/parser.act"

		(ZInode) = ast_make_expr_empty(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2120 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: ast-make-empty */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-alts */
		{
#line 715 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXALTS;
		}
		goto ZL2;
	
#line 2140 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: err-expected-alts */
		/* BEGINNING OF ACTION: ast-make-empty */
		{
#line 854 "src/libre/parser.act"

		(ZInode) = ast_make_expr_empty(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL2;
		}
	
#line 2152 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: ast-make-empty */
	}
	goto ZL0;
ZL2:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_195(flags flags, lex_state lex_state, act_state act_state, err err, t_pos *ZIstart, t_char *ZIcbrak, t_ast__expr *ZOnode1)
{
	t_ast__expr ZInode1;

	switch (CURRENT_TERMINAL) {
	default:
		{
			/* BEGINNING OF ACTION: ast-make-literal */
			{
#line 875 "src/libre/parser.act"

		(ZInode1) = ast_make_expr_literal(act_state->poolp, *flags, (*ZIcbrak));
		if ((ZInode1) == NULL) {
			goto ZL1;
		}
	
#line 2181 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: ast-make-literal */
		}
		break;
	case (TOK_RANGE):
		{
			t_endpoint ZIr;
			t_char ZI196;
			t_pos ZI197;
			t_pos ZI198;
			t_endpoint ZIupper;
			t_pos ZIend;
			t_endpoint ZIlower;

			/* BEGINNING OF ACTION: ast-range-endpoint-literal */
			{
#line 840 "src/libre/parser.act"

		(ZIr).type = AST_ENDPOINT_LITERAL;
		(ZIr).u.literal.c = (unsigned char) (*ZIcbrak);
	
#line 2203 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 309 "src/libre/parser.act"

		ZI196 = '-';
		ZI197 = lex_state->lx.start;
		ZI198   = lex_state->lx.end;

		(void) ZI196;
		(void) ZI197;
		(void) ZI198;
	
#line 2218 "src/libre/dialect/native/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_Hend (flags, lex_state, act_state, err, &ZIupper, &ZIend);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: ast-range-endpoint-literal */
			{
#line 840 "src/libre/parser.act"

		(ZIlower).type = AST_ENDPOINT_LITERAL;
		(ZIlower).u.literal.c = (unsigned char) (*ZIcbrak);
	
#line 2234 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
			/* BEGINNING OF ACTION: ast-make-range */
			{
#line 1007 "src/libre/parser.act"

		unsigned char lower, upper;

		if ((ZIlower).type != AST_ENDPOINT_LITERAL ||
			(ZIupper).type != AST_ENDPOINT_LITERAL) {
			err->e = RE_EUNSUPPORTED;
			goto ZL1;
		}

		lower = (ZIlower).u.literal.c;
		upper = (ZIupper).u.literal.c;

		if (lower > upper) {
			char a[5], b[5];
			
			assert(sizeof err->set >= 1 + sizeof a + 1 + sizeof b + 1 + 1);
			
			sprintf(err->set, "%s-%s",
				escchar(a, sizeof a, lower), escchar(b, sizeof b, upper));
			err->e = RE_ENEGRANGE;
			goto ZL1;
		}

		(ZInode1) = ast_make_expr_range(act_state->poolp, *flags, &(ZIlower), &(ZIupper));
		if ((ZInode1) == NULL) {
			goto ZL1;
		}
	
#line 2268 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: ast-make-range */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode1 = ZInode1;
}

void
p_re__native(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_group__id ZIid;
		t_ast__expr ZIe;

		/* BEGINNING OF ACTION: make-group-id */
		{
#line 882 "src/libre/parser.act"

		(ZIid) = act_state->group_id++;
	
#line 2302 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: make-group-id */
		p_expr (flags, lex_state, act_state, err, &ZIe);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
		/* BEGINNING OF ACTION: ast-make-group */
		{
#line 912 "src/libre/parser.act"

		(ZInode) = ast_make_expr_group(act_state->poolp, *flags, (ZIe), (ZIid));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2319 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: ast-make-group */
		/* BEGINNING OF INLINE: 231 */
		{
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_EOF):
					break;
				default:
					goto ZL3;
				}
				ADVANCE_LEXER;
			}
			goto ZL2;
		ZL3:;
			{
				/* BEGINNING OF ACTION: err-expected-eof */
				{
#line 757 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXEOF;
		}
		goto ZL1;
	
#line 2345 "src/libre/dialect/native/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL2:;
		}
		/* END OF INLINE: 231 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint(flags flags, lex_state lex_state, act_state act_state, err err, t_endpoint *ZOr, t_pos *ZOstart, t_pos *ZOend)
{
	t_endpoint ZIr;
	t_pos ZIstart;
	t_pos ZIend;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 144 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_NAMED__CLASS):
				{
					p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_C_Crange_Hendpoint_Hclass (flags, lex_state, act_state, err, &ZIr, &ZIstart, &ZIend);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
				{
					p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_C_Crange_Hendpoint_Hliteral (flags, lex_state, act_state, err, &ZIr, &ZIstart, &ZIend);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 144 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOr = ZIr;
	*ZOstart = ZIstart;
	*ZOend = ZIend;
}

static void
p_expr_C_Clist_Hof_Halts(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr ZIalts)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_expr_C_Clist_Hof_Halts:;
	{
		t_ast__expr ZIa;

		p_expr_C_Calt (flags, lex_state, act_state, err, &ZIa);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
		/* BEGINNING OF ACTION: ast-add-alt */
		{
#line 1047 "src/libre/parser.act"

		if (!ast_add_expr_alt((ZIalts), (ZIa))) {
			goto ZL1;
		}
	
#line 2432 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: ast-add-alt */
		/* BEGINNING OF INLINE: 229 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ALT):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF INLINE: expr::list-of-alts */
					goto ZL2_expr_C_Clist_Hof_Halts;
					/* END OF INLINE: expr::list-of-alts */
				}
				/*UNREACHED*/
			default:
				break;
			}
		}
		/* END OF INLINE: 229 */
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-alts */
		{
#line 715 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXALTS;
		}
		goto ZL4;
	
#line 2464 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: err-expected-alts */
	}
	goto ZL0;
ZL4:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
}

static void
p_expr_C_Cpiece_C_Ccount(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__count *ZOc)
{
	t_ast__count ZIc;

	switch (CURRENT_TERMINAL) {
	case (TOK_OPENCOUNT):
		{
			t_pos ZI263;
			t_pos ZI264;
			t_unsigned ZIm;

			/* BEGINNING OF EXTRACT: OPENCOUNT */
			{
#line 371 "src/libre/parser.act"

		ZI263 = lex_state->lx.start;
		ZI264   = lex_state->lx.end;

		(void) ZI263;
		(void) ZI264;
	
#line 2497 "src/libre/dialect/native/parser.c"
			}
			/* END OF EXTRACT: OPENCOUNT */
			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (TOK_COUNT):
				/* BEGINNING OF EXTRACT: COUNT */
				{
#line 636 "src/libre/parser.act"

		unsigned long u;
		char *e;

		u = strtoul(lex_state->buf.a, &e, 10);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UINT_MAX) {
			err->e = RE_ECOUNTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL1;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXCOUNT;
			goto ZL1;
		}

		ZIm = (unsigned int) u;
	
#line 2525 "src/libre/dialect/native/parser.c"
				}
				/* END OF EXTRACT: COUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			p_265 (flags, lex_state, act_state, err, &ZI263, &ZIm, &ZIc);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_OPT):
		{
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: count-zero-or-one */
			{
#line 817 "src/libre/parser.act"

		(ZIc) = ast_make_count(0, 1);
	
#line 2549 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: count-zero-or-one */
		}
		break;
	case (TOK_PLUS):
		{
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: count-one-or-more */
			{
#line 813 "src/libre/parser.act"

		(ZIc) = ast_make_count(1, AST_COUNT_UNBOUNDED);
	
#line 2563 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: count-one-or-more */
		}
		break;
	case (TOK_STAR):
		{
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: count-zero-or-more */
			{
#line 809 "src/libre/parser.act"

		(ZIc) = ast_make_count(0, AST_COUNT_UNBOUNDED);
	
#line 2577 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: count-zero-or-more */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: count-one */
			{
#line 821 "src/libre/parser.act"

		(ZIc) = ast_make_count(1, 1);
	
#line 2590 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: count-one */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-count */
		{
#line 701 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCOUNT;
		}
		goto ZL2;
	
#line 2610 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: err-expected-count */
		/* BEGINNING OF ACTION: count-one */
		{
#line 821 "src/libre/parser.act"

		(ZIc) = ast_make_count(1, 1);
	
#line 2619 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: count-one */
	}
	goto ZL0;
ZL2:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOc = ZIc;
}

static void
p_expr_C_Cpiece_C_Catom(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOe)
{
	t_ast__expr ZIe;

	switch (CURRENT_TERMINAL) {
	case (TOK_ANY):
		{
			t_ast__class__id ZIa;

			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: class-any */
			{
#line 784 "src/libre/parser.act"

		/* TODO: or the unicode equivalent */
		(ZIa) = (*flags & RE_SINGLE) ? &class_any : &class_notnl;
	
#line 2649 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: class-any */
			/* BEGINNING OF ACTION: ast-make-named */
			{
#line 1034 "src/libre/parser.act"

		(ZIe) = ast_make_expr_named(act_state->poolp, *flags, (ZIa));
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 2661 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: ast-make-named */
		}
		break;
	case (TOK_END):
		{
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-make-anchor-end */
			{
#line 943 "src/libre/parser.act"

		(ZIe) = ast_make_expr_anchor(act_state->poolp, *flags, AST_ANCHOR_END);
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 2678 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: ast-make-anchor-end */
		}
		break;
	case (TOK_OPENSUB):
		{
			t_group__id ZIid;
			t_ast__expr ZIg;

			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: make-group-id */
			{
#line 882 "src/libre/parser.act"

		(ZIid) = act_state->group_id++;
	
#line 2695 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: make-group-id */
			p_expr (flags, lex_state, act_state, err, &ZIg);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: ast-make-group */
			{
#line 912 "src/libre/parser.act"

		(ZIe) = ast_make_expr_group(act_state->poolp, *flags, (ZIg), (ZIid));
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 2712 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: ast-make-group */
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSESUB):
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
		}
		break;
	case (TOK_START):
		{
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-make-anchor-start */
			{
#line 936 "src/libre/parser.act"

		(ZIe) = ast_make_expr_anchor(act_state->poolp, *flags, AST_ANCHOR_START);
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 2736 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: ast-make-anchor-start */
		}
		break;
	case (TOK_OPENGROUP): case (TOK_OPENGROUPINV): case (TOK_OPENGROUPCB): case (TOK_OPENGROUPINVCB):
		{
			p_expr_C_Ccharacter_Hclass (flags, lex_state, act_state, err, &ZIe);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_ESC): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
		{
			p_expr_C_Cliteral (flags, lex_state, act_state, err, &ZIe);
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
	{
		/* BEGINNING OF ACTION: err-expected-atom */
		{
#line 708 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXATOM;
		}
		goto ZL2;
	
#line 2776 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: err-expected-atom */
		/* BEGINNING OF ACTION: ast-make-empty */
		{
#line 854 "src/libre/parser.act"

		(ZIe) = ast_make_expr_empty(act_state->poolp, *flags);
		if ((ZIe) == NULL) {
			goto ZL2;
		}
	
#line 2788 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: ast-make-empty */
	}
	goto ZL0;
ZL2:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOe = ZIe;
}

static void
p_246(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__class__id *ZI243, t_pos *ZI244, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	default:
		{
			/* BEGINNING OF ACTION: ast-make-named */
			{
#line 1034 "src/libre/parser.act"

		(ZInode) = ast_make_expr_named(act_state->poolp, *flags, (*ZI243));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2817 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: ast-make-named */
		}
		break;
	case (TOK_RANGE):
		{
			t_endpoint ZIlower;
			t_endpoint ZIupper;
			t_pos ZIend;

			/* BEGINNING OF ACTION: ast-range-endpoint-class */
			{
#line 845 "src/libre/parser.act"

		(ZIlower).type = AST_ENDPOINT_NAMED;
		(ZIlower).u.named.class = (*ZI243);
	
#line 2835 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-class */
			p_154 (flags, lex_state, act_state, err);
			p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_Hend (flags, lex_state, act_state, err, &ZIupper, &ZIend);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: mark-range */
			{
#line 773 "src/libre/parser.act"

		mark(&act_state->rangestart, &(*ZI244));
		mark(&act_state->rangeend,   &(ZIend));
	
#line 2851 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: mark-range */
			/* BEGINNING OF ACTION: ast-make-range */
			{
#line 1007 "src/libre/parser.act"

		unsigned char lower, upper;

		if ((ZIlower).type != AST_ENDPOINT_LITERAL ||
			(ZIupper).type != AST_ENDPOINT_LITERAL) {
			err->e = RE_EUNSUPPORTED;
			goto ZL1;
		}

		lower = (ZIlower).u.literal.c;
		upper = (ZIupper).u.literal.c;

		if (lower > upper) {
			char a[5], b[5];
			
			assert(sizeof err->set >= 1 + sizeof a + 1 + sizeof b + 1 + 1);
			
			sprintf(err->set, "%s-%s",
				escchar(a, sizeof a, lower), escchar(b, sizeof b, upper));
			err->e = RE_ENEGRANGE;
			goto ZL1;
		}

		(ZInode) = ast_make_expr_range(act_state->poolp, *flags, &(ZIlower), &(ZIupper));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2885 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: ast-make-range */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Calt(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF ACTION: ast-make-concat */
		{
#line 861 "src/libre/parser.act"

		(ZInode) = ast_make_expr_concat(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2919 "src/libre/dialect/native/parser.c"
		}
		/* END OF ACTION: ast-make-concat */
		p_expr_C_Clist_Hof_Hpieces (flags, lex_state, act_state, err, ZInode);
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
	*ZOnode = ZInode;
}

static void
p_250(flags flags, lex_state lex_state, act_state act_state, err err, t_char *ZI247, t_pos *ZI248, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	default:
		{
			/* BEGINNING OF ACTION: ast-make-literal */
			{
#line 875 "src/libre/parser.act"

		(ZInode) = ast_make_expr_literal(act_state->poolp, *flags, (*ZI247));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2953 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: ast-make-literal */
		}
		break;
	case (TOK_RANGE):
		{
			t_endpoint ZIlower;
			t_endpoint ZIupper;
			t_pos ZIend;

			/* BEGINNING OF ACTION: ast-range-endpoint-literal */
			{
#line 840 "src/libre/parser.act"

		(ZIlower).type = AST_ENDPOINT_LITERAL;
		(ZIlower).u.literal.c = (unsigned char) (*ZI247);
	
#line 2971 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
			p_154 (flags, lex_state, act_state, err);
			p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_Hend (flags, lex_state, act_state, err, &ZIupper, &ZIend);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: mark-range */
			{
#line 773 "src/libre/parser.act"

		mark(&act_state->rangestart, &(*ZI248));
		mark(&act_state->rangeend,   &(ZIend));
	
#line 2987 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: mark-range */
			/* BEGINNING OF ACTION: ast-make-range */
			{
#line 1007 "src/libre/parser.act"

		unsigned char lower, upper;

		if ((ZIlower).type != AST_ENDPOINT_LITERAL ||
			(ZIupper).type != AST_ENDPOINT_LITERAL) {
			err->e = RE_EUNSUPPORTED;
			goto ZL1;
		}

		lower = (ZIlower).u.literal.c;
		upper = (ZIupper).u.literal.c;

		if (lower > upper) {
			char a[5], b[5];
			
			assert(sizeof err->set >= 1 + sizeof a + 1 + sizeof b + 1 + 1);
			
			sprintf(err->set, "%s-%s",
				escchar(a, sizeof a, lower), escchar(b, sizeof b, upper));
			err->e = RE_ENEGRANGE;
			goto ZL1;
		}

		(ZInode) = ast_make_expr_range(act_state->poolp, *flags, &(ZIlower), &(ZIupper));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 3021 "src/libre/dialect/native/parser.c"
			}
			/* END OF ACTION: ast-make-range */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

/* BEGINNING OF TRAILER */

#line 1207 "src/libre/parser.act"


	static int
	lgetc(struct LX_STATE *lx)
	{
		struct lex_state *lex_state;

		assert(lx != NULL);
		assert(lx->getc_opaque != NULL);

		lex_state = lx->getc_opaque;

		assert(lex_state->f != NULL);

		return lex_state->f(lex_state->opaque);
	}

	struct ast *
	DIALECT_PARSE(re_getchar_fun *f, void *opaque,
		enum re_flags flags, int overlap,
		struct re_err *err, struct re_pos *end)
	{
		struct ast *ast;

		struct act_state  act_state_s;
		struct act_state *act_state;
		struct lex_state  lex_state_s;
		struct lex_state *lex_state;
		struct re_err dummy;

		struct LX_STATE *lx;

		assert(f != NULL);
		assert(end != NULL);

		ast = ast_new();

		if (err == NULL) {
			err = &dummy;
		}

		lex_state    = &lex_state_s;
		lex_state->p = lex_state->a;

		lx = &lex_state->lx;

		LX_INIT(lx);

		lx->lgetc       = lgetc;
		lx->getc_opaque = lex_state;

		lex_state->f       = f;
		lex_state->opaque  = opaque;

		lex_state->buf.a   = NULL;
		lex_state->buf.len = 0;

		/* XXX: unneccessary since we're lexing from a string */
		/* (except for pushing "[" and "]" around ::group-$dialect) */
		lx->buf_opaque = &lex_state->buf;
		lx->push       = CAT(LX_PREFIX, _dynpush);
		lx->clear      = CAT(LX_PREFIX, _dynclear);
		lx->free       = CAT(LX_PREFIX, _dynfree);

		/* This is a workaround for ADVANCE_LEXER assuming a pointer */
		act_state = &act_state_s;

		act_state->overlap = overlap;
		act_state->poolp   = &ast->pool;

		act_state->group_id = 0;

		err->e = RE_ESUCCESS;

		ADVANCE_LEXER;

#if BUILD_FOR_FUZZER
		/* these errors currently are not handled properly */
		if (act_state->lex_tok == TOK_ERROR) {
			fprintf(stderr, "syntax error\n");
			lx->free(lx->buf_opaque);
			goto error;
		}
#endif

		DIALECT_ENTRY(&flags, lex_state, act_state, err, &ast->expr);

		/* we output this for reporting errors found through AST analysis */
		mark(end, &lx->end);

		lx->free(lx->buf_opaque);

		if (err->e != RE_ESUCCESS) {
			/* TODO: free internals allocated during parsing (are there any?) */
			goto error;
		}

		if (ast->expr == NULL) {
			/* We shouldn't get here, it means there's error
			 * checking missing elsewhere. */
			if (err->e == RE_ESUCCESS) { assert(!"unreached"); }
			goto error;
		}

		return ast;

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

		ast_free(ast);

		return NULL;
	}

#line 3195 "src/libre/dialect/native/parser.c"

/* END OF FILE */
