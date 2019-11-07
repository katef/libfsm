/*
 * Automatically generated from the files:
 *	src/libre/dialect/pcre/parser.sid
 * and
 *	src/libre/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 144 "src/libre/parser.act"


	#include <assert.h>
	#include <limits.h>
	#include <string.h>
	#include <stdlib.h>
	#include <stdio.h>
	#include <errno.h>
	#include <ctype.h>

	#include <re/re.h>

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

	/* This is a hack to work around the AST files not being able to include lexer.h. */
	#define AST_POS_OF_LX_POS(AST_POS, LX_POS) \
	    do {                                   \
	        AST_POS.line = LX_POS.line;        \
	        AST_POS.col = LX_POS.col;          \
	        AST_POS.byte = LX_POS.byte;        \
	    } while (0)

	#include "parser.h"
	#include "lexer.h"

	#include "../comp.h"
	#include "../../class.h"

	struct flags {
		enum re_flags flags;
		struct flags *parent;
	};

	typedef char     t_char;
	typedef unsigned t_unsigned;
	typedef unsigned t_pred; /* TODO */

	typedef struct lx_pos t_pos;
	typedef enum re_flags t_re__flags;
	typedef class_constructor * t_ast__class__id;
	typedef struct ast_count t_ast__count;
	typedef struct ast_endpoint t_endpoint;

	typedef struct ast_class * t_ast__class;
	typedef enum ast_class_flags t_class__flags;

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

		re_getchar_fun *f;
		void *opaque;

		/* TODO: use lx's generated conveniences for the pattern buffer */
		char a[512];
		char *p;
	};

	#define CURRENT_TERMINAL (act_state->lex_tok)
	#define ERROR_TERMINAL   (TOK_ERROR)
	#define ADVANCE_LEXER    do { mark(&act_state->synstart, &lex_state->lx.start); \
	                              mark(&act_state->synend,   &lex_state->lx.end);   \
	                              act_state->lex_tok = LX_NEXT(&lex_state->lx); \
		} while (0)
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

#line 215 "src/libre/dialect/pcre/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_expr_C_Ccharacter_Hclass_C_Cclass_Hhead(flags, lex_state, act_state, err, t_ast__class);
static void p_expr_C_Cflags_C_Cflag__set(flags, lex_state, act_state, err, t_re__flags, t_re__flags *);
static void p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms(flags, lex_state, act_state, err, t_ast__class);
static void p_146(flags, lex_state, act_state, err);
static void p_150(flags, lex_state, act_state, err, t_endpoint *, t_pos *);
static void p_expr_C_Cliteral(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Ccharacter_Hclass_C_Cclass_Hterm(flags, lex_state, act_state, err, t_ast__class *);
static void p_expr_C_Ccharacter_Hclass(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Ccharacter_Hclass_C_Cclass_Hrange_C_Crange_Hendpoint_Hliteral(flags, lex_state, act_state, err, t_endpoint *, t_pos *, t_pos *);
static void p_190(flags, lex_state, act_state, err);
static void p_expr(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Cflags(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Clist_Hof_Hatoms(flags, lex_state, act_state, err, t_ast__expr);
static void p_expr_C_Clist_Hof_Halts(flags, lex_state, act_state, err, t_ast__expr);
static void p_expr_C_Ccharacter_Hclass_C_Cclass_Hrange_C_Crange_Hendpoint_Hclass(flags, lex_state, act_state, err, t_endpoint *, t_pos *, t_pos *);
static void p_227(flags, lex_state, act_state, err, t_ast__class__id *, t_pos *, t_ast__class *);
extern void p_re__pcre(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Catom(flags, lex_state, act_state, err, t_ast__expr *);
static void p_247(flags, lex_state, act_state, err, t_char *, t_pos *, t_ast__class *);
static void p_expr_C_Calt(flags, lex_state, act_state, err, t_ast__expr *);
static void p_250(flags, lex_state, act_state, err, t_ast__expr *, t_pos *, t_unsigned *, t_ast__expr *);
static void p_expr_C_Ctype(flags, lex_state, act_state, err, t_ast__expr *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_expr_C_Ccharacter_Hclass_C_Cclass_Hhead(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__class ZIclass)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_INVERT):
		{
			t_char ZI112;
			t_ast__class ZIf;

			/* BEGINNING OF EXTRACT: INVERT */
			{
#line 239 "src/libre/parser.act"

		ZI112 = '^';
	
#line 267 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: INVERT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-make-class-flag-invert */
			{
#line 809 "src/libre/parser.act"

		(ZIf) = ast_make_class_flags(AST_CLASS_FLAG_INVERTED);
		if ((ZIf) == NULL) {
			goto ZL1;
		}
	
#line 280 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-class-flag-invert */
			/* BEGINNING OF ACTION: ast-add-class-concat */
			{
#line 830 "src/libre/parser.act"

		if (!ast_add_class_concat((ZIclass), (ZIf))) {
			goto ZL1;
		}
	
#line 291 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-add-class-concat */
		}
		break;
	default:
		{
			t_ast__class ZIf;

			/* BEGINNING OF ACTION: ast-make-class-flag-none */
			{
#line 802 "src/libre/parser.act"

		(ZIf) = ast_make_class_flags(AST_CLASS_FLAG_NONE);
		if ((ZIf) == NULL) {
			goto ZL1;
		}
	
#line 309 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-class-flag-none */
			/* BEGINNING OF ACTION: ast-add-class-concat */
			{
#line 830 "src/libre/parser.act"

		if (!ast_add_class_concat((ZIclass), (ZIf))) {
			goto ZL1;
		}
	
#line 320 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-add-class-concat */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_expr_C_Cflags_C_Cflag__set(flags flags, lex_state lex_state, act_state act_state, err err, t_re__flags ZIi, t_re__flags *ZOo)
{
	t_re__flags ZIo;

	switch (CURRENT_TERMINAL) {
	case (TOK_FLAG__INSENSITIVE):
		{
			t_re__flags ZIc;

			/* BEGINNING OF EXTRACT: FLAG_INSENSITIVE */
			{
#line 439 "src/libre/parser.act"

		ZIc = RE_ICASE;
	
#line 350 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: FLAG_INSENSITIVE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: re-flag-union */
			{
#line 554 "src/libre/parser.act"

		(ZIo) = (ZIi) | (ZIc);
	
#line 360 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: re-flag-union */
		}
		break;
	case (TOK_FLAG__UNKNOWN):
		{
			ADVANCE_LEXER;
			ZIo = ZIi;
			/* BEGINNING OF ACTION: err-unknown-flag */
			{
#line 505 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EFLAG;
		}
		goto ZL1;
	
#line 378 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: err-unknown-flag */
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
	*ZOo = ZIo;
}

static void
p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__class ZIclass)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms:;
	{
		/* BEGINNING OF INLINE: 159 */
		{
			{
				t_ast__class ZInode;

				p_expr_C_Ccharacter_Hclass_C_Cclass_Hterm (flags, lex_state, act_state, err, &ZInode);
				if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
					RESTORE_LEXER;
					goto ZL4;
				}
				/* BEGINNING OF ACTION: ast-add-class-concat */
				{
#line 830 "src/libre/parser.act"

		if (!ast_add_class_concat((ZIclass), (ZInode))) {
			goto ZL4;
		}
	
#line 422 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: ast-add-class-concat */
			}
			goto ZL3;
		ZL4:;
			{
				/* BEGINNING OF ACTION: err-expected-term */
				{
#line 456 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXTERM;
		}
		goto ZL1;
	
#line 438 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-term */
			}
		ZL3:;
		}
		/* END OF INLINE: 159 */
		/* BEGINNING OF INLINE: 160 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_NAMED__CLASS): case (TOK_ESC): case (TOK_CONTROL): case (TOK_OCT):
			case (TOK_HEX): case (TOK_CHAR):
				{
					/* BEGINNING OF INLINE: expr::character-class::list-of-class-terms */
					goto ZL2_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms;
					/* END OF INLINE: expr::character-class::list-of-class-terms */
				}
				/*UNREACHED*/
			default:
				break;
			}
		}
		/* END OF INLINE: 160 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_146(flags flags, lex_state lex_state, act_state act_state, err err)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZI147;
		t_pos ZI148;
		t_pos ZI149;

		switch (CURRENT_TERMINAL) {
		case (TOK_RANGE):
			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 243 "src/libre/parser.act"

		ZI147 = '-';
		ZI148 = lex_state->lx.start;
		ZI149   = lex_state->lx.end;
	
#line 489 "src/libre/dialect/pcre/parser.c"
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
#line 484 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXRANGE;
		}
		goto ZL2;
	
#line 510 "src/libre/dialect/pcre/parser.c"
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
p_150(flags flags, lex_state lex_state, act_state act_state, err err, t_endpoint *ZOupper, t_pos *ZOend)
{
	t_endpoint ZIupper;
	t_pos ZIend;

	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZIc;
			t_pos ZI154;

			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 243 "src/libre/parser.act"

		ZIc = '-';
		ZI154 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 541 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-range-endpoint-literal */
			{
#line 594 "src/libre/parser.act"

		(ZIupper).type = AST_ENDPOINT_LITERAL;
		(ZIupper).u.literal.c = (ZIc);
	
#line 552 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
		}
		break;
	case (TOK_NAMED__CLASS):
		{
			t_pos ZI153;

			p_expr_C_Ccharacter_Hclass_C_Cclass_Hrange_C_Crange_Hendpoint_Hclass (flags, lex_state, act_state, err, &ZIupper, &ZI153, &ZIend);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_ESC): case (TOK_CONTROL): case (TOK_OCT): case (TOK_HEX):
	case (TOK_CHAR):
		{
			t_pos ZI152;

			p_expr_C_Ccharacter_Hclass_C_Cclass_Hrange_C_Crange_Hendpoint_Hliteral (flags, lex_state, act_state, err, &ZIupper, &ZI152, &ZIend);
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
	*ZOupper = ZIupper;
	*ZOend = ZIend;
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

		/* BEGINNING OF INLINE: 97 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					t_pos ZI105;
					t_pos ZI106;

					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 401 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI105 = lex_state->lx.start;
		ZI106   = lex_state->lx.end;

		ZIc = lex_state->buf.a[0];
	
#line 625 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_ESC):
				{
					t_pos ZI99;
					t_pos ZI100;

					/* BEGINNING OF EXTRACT: ESC */
					{
#line 274 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZIc = lex_state->buf.a[1];

		switch (ZIc) {
		case 'a': ZIc = '\a'; break;
		case 'f': ZIc = '\f'; break;
		case 'n': ZIc = '\n'; break;
		case 'r': ZIc = '\r'; break;
		case 't': ZIc = '\t'; break;
		case 'v': ZIc = '\v'; break;
		default:             break;
		}

		ZI99 = lex_state->lx.start;
		ZI100   = lex_state->lx.end;
	
#line 659 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: ESC */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_HEX):
				{
					t_pos ZI103;
					t_pos ZI104;

					/* BEGINNING OF EXTRACT: HEX */
					{
#line 365 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 3);

		ZI103 = lex_state->lx.start;
		ZI104   = lex_state->lx.end;

		errno = 0;

		s = lex_state->buf.a + 2;

		if (*s == '{') {
			s++;
			brace = 1;
		}

		u = strtoul(s, &e, 16);

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
	
#line 712 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: HEX */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_OCT):
				{
					t_pos ZI101;
					t_pos ZI102;

					/* BEGINNING OF EXTRACT: OCT */
					{
#line 325 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		ZI101 = lex_state->lx.start;
		ZI102   = lex_state->lx.end;

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
	
#line 765 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OCT */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 97 */
		/* BEGINNING OF ACTION: ast-make-expr-literal */
		{
#line 646 "src/libre/parser.act"

		(ZInode) = ast_make_expr_literal((ZIc));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 785 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-make-expr-literal */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Ccharacter_Hclass_C_Cclass_Hterm(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__class *ZOnode)
{
	t_ast__class ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_CHAR):
		{
			t_char ZI240;
			t_pos ZI241;
			t_pos ZI242;

			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 401 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI241 = lex_state->lx.start;
		ZI242   = lex_state->lx.end;

		ZI240 = lex_state->buf.a[0];
	
#line 821 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			ADVANCE_LEXER;
			p_247 (flags, lex_state, act_state, err, &ZI240, &ZI241, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_CONTROL):
		{
			t_char ZI244;
			t_pos ZI245;
			t_pos ZI246;

			/* BEGINNING OF EXTRACT: CONTROL */
			{
#line 307 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] == 'c');
		assert(lex_state->buf.a[2] != '\0');
		assert(lex_state->buf.a[3] == '\0');

		ZI244 = lex_state->buf.a[2];
		if ((unsigned char) ZI244 > 127) {
			goto ZL1;
		}
		ZI244 = (((toupper((unsigned char)ZI244)) - 64) % 128 + 128) % 128;

		ZI245 = lex_state->lx.start;
		ZI246   = lex_state->lx.end;
	
#line 856 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: CONTROL */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: err-unsupported */
			{
#line 526 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXUNSUPPORTD;
		}
		goto ZL1;
	
#line 869 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: err-unsupported */
			p_247 (flags, lex_state, act_state, err, &ZI244, &ZI245, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_ESC):
		{
			t_char ZI228;
			t_pos ZI229;
			t_pos ZI230;

			/* BEGINNING OF EXTRACT: ESC */
			{
#line 274 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZI228 = lex_state->buf.a[1];

		switch (ZI228) {
		case 'a': ZI228 = '\a'; break;
		case 'f': ZI228 = '\f'; break;
		case 'n': ZI228 = '\n'; break;
		case 'r': ZI228 = '\r'; break;
		case 't': ZI228 = '\t'; break;
		case 'v': ZI228 = '\v'; break;
		default:             break;
		}

		ZI229 = lex_state->lx.start;
		ZI230   = lex_state->lx.end;
	
#line 908 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: ESC */
			ADVANCE_LEXER;
			p_247 (flags, lex_state, act_state, err, &ZI228, &ZI229, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_HEX):
		{
			t_char ZI236;
			t_pos ZI237;
			t_pos ZI238;

			/* BEGINNING OF EXTRACT: HEX */
			{
#line 365 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 3);

		ZI237 = lex_state->lx.start;
		ZI238   = lex_state->lx.end;

		errno = 0;

		s = lex_state->buf.a + 2;

		if (*s == '{') {
			s++;
			brace = 1;
		}

		u = strtoul(s, &e, 16);

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

		ZI236 = (char) (unsigned char) u;
	
#line 967 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: HEX */
			ADVANCE_LEXER;
			p_247 (flags, lex_state, act_state, err, &ZI236, &ZI237, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_NAMED__CLASS):
		{
			t_ast__class__id ZI224;
			t_pos ZI225;
			t_pos ZI226;

			/* BEGINNING OF EXTRACT: NAMED_CLASS */
			{
#line 428 "src/libre/parser.act"

		ZI224 = DIALECT_CLASS(lex_state->buf.a);
		if (ZI224 == NULL) {
			/* syntax error -- unrecognized class */
			goto ZL1;
		}

		ZI225 = lex_state->lx.start;
		ZI226   = lex_state->lx.end;
	
#line 997 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: NAMED_CLASS */
			ADVANCE_LEXER;
			p_227 (flags, lex_state, act_state, err, &ZI224, &ZI225, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_OCT):
		{
			t_char ZI232;
			t_pos ZI233;
			t_pos ZI234;

			/* BEGINNING OF EXTRACT: OCT */
			{
#line 325 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		ZI233 = lex_state->lx.start;
		ZI234   = lex_state->lx.end;

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

		ZI232 = (char) (unsigned char) u;
	
#line 1056 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: OCT */
			ADVANCE_LEXER;
			p_247 (flags, lex_state, act_state, err, &ZI232, &ZI233, &ZInode);
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
p_expr_C_Ccharacter_Hclass(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_pos ZIstart;
		t_pos ZI161;
		t_ast__class ZIclass;
		t_pos ZIend;

		switch (CURRENT_TERMINAL) {
		case (TOK_OPENGROUP):
			/* BEGINNING OF EXTRACT: OPENGROUP */
			{
#line 249 "src/libre/parser.act"

		ZIstart = lex_state->lx.start;
		ZI161   = lex_state->lx.end;
	
#line 1103 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: OPENGROUP */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: ast-make-class-concat */
		{
#line 741 "src/libre/parser.act"

		(ZIclass) = ast_make_class_concat();
		if ((ZIclass) == NULL) {
			goto ZL1;
		}
	
#line 1120 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-make-class-concat */
		p_expr_C_Ccharacter_Hclass_C_Cclass_Hhead (flags, lex_state, act_state, err, ZIclass);
		p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms (flags, lex_state, act_state, err, ZIclass);
		/* BEGINNING OF INLINE: 162 */
		{
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			{
				t_char ZI163;
				t_pos ZI164;

				switch (CURRENT_TERMINAL) {
				case (TOK_CLOSEGROUP):
					/* BEGINNING OF EXTRACT: CLOSEGROUP */
					{
#line 254 "src/libre/parser.act"

		ZI163 = ']';
		ZI164 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 1145 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CLOSEGROUP */
					break;
				default:
					goto ZL3;
				}
				ADVANCE_LEXER;
				/* BEGINNING OF ACTION: mark-group */
				{
#line 530 "src/libre/parser.act"

		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));
	
#line 1160 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: mark-group */
			}
			goto ZL2;
		ZL3:;
			{
				/* BEGINNING OF ACTION: err-expected-closegroup */
				{
#line 491 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCLOSEGROUP;
		}
		goto ZL1;
	
#line 1176 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-closegroup */
				ZIend = ZIstart;
			}
		ZL2:;
		}
		/* END OF INLINE: 162 */
		/* BEGINNING OF ACTION: ast-make-expr-class */
		{
#line 684 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;

		AST_POS_OF_LX_POS(ast_start, (ZIstart));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));

		(ZInode) = ast_make_expr_class((ZIclass), &ast_start, &ast_end);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1201 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-make-expr-class */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Ccharacter_Hclass_C_Cclass_Hrange_C_Crange_Hendpoint_Hliteral(flags flags, lex_state lex_state, act_state act_state, err err, t_endpoint *ZOr, t_pos *ZOstart, t_pos *ZOend)
{
	t_endpoint ZIr;
	t_pos ZIstart;
	t_pos ZIend;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZIc;

		/* BEGINNING OF INLINE: 139 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 401 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		ZIc = lex_state->buf.a[0];
	
#line 1243 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_CONTROL):
				{
					/* BEGINNING OF EXTRACT: CONTROL */
					{
#line 307 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] == 'c');
		assert(lex_state->buf.a[2] != '\0');
		assert(lex_state->buf.a[3] == '\0');

		ZIc = lex_state->buf.a[2];
		if ((unsigned char) ZIc > 127) {
			goto ZL1;
		}
		ZIc = (((toupper((unsigned char)ZIc)) - 64) % 128 + 128) % 128;

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 1269 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CONTROL */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: err-unsupported */
					{
#line 526 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXUNSUPPORTD;
		}
		goto ZL1;
	
#line 1282 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: err-unsupported */
				}
				break;
			case (TOK_ESC):
				{
					/* BEGINNING OF EXTRACT: ESC */
					{
#line 274 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZIc = lex_state->buf.a[1];

		switch (ZIc) {
		case 'a': ZIc = '\a'; break;
		case 'f': ZIc = '\f'; break;
		case 'n': ZIc = '\n'; break;
		case 'r': ZIc = '\r'; break;
		case 't': ZIc = '\t'; break;
		case 'v': ZIc = '\v'; break;
		default:             break;
		}

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 1312 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: ESC */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_HEX):
				{
					/* BEGINNING OF EXTRACT: HEX */
					{
#line 365 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 3);

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		errno = 0;

		s = lex_state->buf.a + 2;

		if (*s == '{') {
			s++;
			brace = 1;
		}

		u = strtoul(s, &e, 16);

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
	
#line 1362 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: HEX */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_OCT):
				{
					/* BEGINNING OF EXTRACT: OCT */
					{
#line 325 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

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
	
#line 1412 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OCT */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 139 */
		/* BEGINNING OF ACTION: ast-range-endpoint-literal */
		{
#line 594 "src/libre/parser.act"

		(ZIr).type = AST_ENDPOINT_LITERAL;
		(ZIr).u.literal.c = (ZIc);
	
#line 1430 "src/libre/dialect/pcre/parser.c"
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
p_190(flags flags, lex_state lex_state, act_state act_state, err err)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case (TOK_CLOSE):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-alts */
		{
#line 477 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXALTS;
		}
		goto ZL2;
	
#line 1471 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: err-expected-alts */
	}
	goto ZL0;
ZL2:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
}

static void
p_expr(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF ACTION: ast-make-expr-alt */
		{
#line 639 "src/libre/parser.act"

		(ZInode) = ast_make_expr_alt();
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1500 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-make-expr-alt */
		p_expr_C_Clist_Hof_Halts (flags, lex_state, act_state, err, ZInode);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
	}
	goto ZL0;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-alts */
		{
#line 477 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXALTS;
		}
		goto ZL2;
	
#line 1521 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: err-expected-alts */
		/* BEGINNING OF ACTION: ast-make-expr-empty */
		{
#line 625 "src/libre/parser.act"

		(ZInode) = ast_make_expr_empty();
		if ((ZInode) == NULL) {
			goto ZL2;
		}
	
#line 1533 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-make-expr-empty */
	}
	goto ZL0;
ZL2:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Cflags(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_re__flags ZIempty__pos;
		t_re__flags ZIempty__neg;
		t_re__flags ZIpos;
		t_re__flags ZIneg;

		switch (CURRENT_TERMINAL) {
		case (TOK_OPENFLAGS):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: re-flag-none */
		{
#line 550 "src/libre/parser.act"

		(ZIempty__pos) = RE_FLAGS_NONE;
	
#line 1572 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: re-flag-none */
		/* BEGINNING OF ACTION: re-flag-none */
		{
#line 550 "src/libre/parser.act"

		(ZIempty__neg) = RE_FLAGS_NONE;
	
#line 1581 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: re-flag-none */
		/* BEGINNING OF INLINE: 178 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_FLAG__UNKNOWN): case (TOK_FLAG__INSENSITIVE):
				{
					p_expr_C_Cflags_C_Cflag__set (flags, lex_state, act_state, err, ZIempty__pos, &ZIpos);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				{
					ZIpos = ZIempty__pos;
				}
				break;
			}
		}
		/* END OF INLINE: 178 */
		/* BEGINNING OF INLINE: 180 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_NEGATE):
				{
					ADVANCE_LEXER;
					p_expr_C_Cflags_C_Cflag__set (flags, lex_state, act_state, err, ZIempty__neg, &ZIneg);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				{
					ZIneg = ZIempty__neg;
				}
				break;
			}
		}
		/* END OF INLINE: 180 */
		/* BEGINNING OF INLINE: 183 */
		{
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_CLOSEFLAGS):
					break;
				default:
					goto ZL5;
				}
				ADVANCE_LEXER;
				/* BEGINNING OF ACTION: ast-make-expr-re-flags */
				{
#line 704 "src/libre/parser.act"

		(ZInode) = ast_make_expr_re_flags((ZIpos), (ZIneg));
		if ((ZInode) == NULL) {
			goto ZL5;
		}
	
#line 1644 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: ast-make-expr-re-flags */
			}
			goto ZL4;
		ZL5:;
			{
				/* BEGINNING OF ACTION: err-expected-closeflags */
				{
#line 512 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCLOSEFLAGS;
		}
		goto ZL1;
	
#line 1660 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-closeflags */
				/* BEGINNING OF ACTION: ast-make-expr-empty */
				{
#line 625 "src/libre/parser.act"

		(ZInode) = ast_make_expr_empty();
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1672 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: ast-make-expr-empty */
			}
		ZL4:;
		}
		/* END OF INLINE: 183 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Clist_Hof_Hatoms(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr ZIcat)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_expr_C_Clist_Hof_Hatoms:;
	{
		t_ast__expr ZIa;

		p_expr_C_Catom (flags, lex_state, act_state, err, &ZIa);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
		/* BEGINNING OF ACTION: ast-add-expr-concat */
		{
#line 725 "src/libre/parser.act"

		if (!ast_add_expr_concat((ZIcat), (ZIa))) {
			goto ZL1;
		}
	
#line 1711 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-add-expr-concat */
		/* BEGINNING OF INLINE: 206 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY): case (TOK_START): case (TOK_END): case (TOK_OPENSUB):
			case (TOK_OPENCAPTURE): case (TOK_OPENGROUP): case (TOK_NAMED__CLASS): case (TOK_OPENFLAGS):
			case (TOK_ESC): case (TOK_CONTROL): case (TOK_OCT): case (TOK_HEX):
			case (TOK_CHAR):
				{
					/* BEGINNING OF INLINE: expr::list-of-atoms */
					goto ZL2_expr_C_Clist_Hof_Hatoms;
					/* END OF INLINE: expr::list-of-atoms */
				}
				/*UNREACHED*/
			default:
				break;
			}
		}
		/* END OF INLINE: 206 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
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
		/* BEGINNING OF ACTION: ast-add-expr-alt */
		{
#line 731 "src/libre/parser.act"

		if (!ast_add_expr_alt((ZIalts), (ZIa))) {
			goto ZL1;
		}
	
#line 1762 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-add-expr-alt */
		/* BEGINNING OF INLINE: 212 */
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
		/* END OF INLINE: 212 */
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-alts */
		{
#line 477 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXALTS;
		}
		goto ZL4;
	
#line 1794 "src/libre/dialect/pcre/parser.c"
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
p_expr_C_Ccharacter_Hclass_C_Cclass_Hrange_C_Crange_Hendpoint_Hclass(flags flags, lex_state lex_state, act_state act_state, err err, t_endpoint *ZOr, t_pos *ZOstart, t_pos *ZOend)
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
#line 428 "src/libre/parser.act"

		ZIid = DIALECT_CLASS(lex_state->buf.a);
		if (ZIid == NULL) {
			/* syntax error -- unrecognized class */
			goto ZL1;
		}

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 1833 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: NAMED_CLASS */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: ast-range-endpoint-class */
		{
#line 599 "src/libre/parser.act"

		(ZIr).type = AST_ENDPOINT_CLASS;
		(ZIr).u.class.ctor = (ZIid);
	
#line 1848 "src/libre/dialect/pcre/parser.c"
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
p_227(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__class__id *ZI224, t_pos *ZI225, t_ast__class *ZOnode)
{
	t_ast__class ZInode;

	switch (CURRENT_TERMINAL) {
	default:
		{
			/* BEGINNING OF ACTION: ast-make-class-named */
			{
#line 788 "src/libre/parser.act"

		(ZInode) = ast_make_class_named((*ZI224));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1879 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-class-named */
		}
		break;
	case (TOK_RANGE):
		{
			t_endpoint ZIlower;
			t_endpoint ZIupper;
			t_pos ZIend;

			/* BEGINNING OF ACTION: ast-range-endpoint-class */
			{
#line 599 "src/libre/parser.act"

		(ZIlower).type = AST_ENDPOINT_CLASS;
		(ZIlower).u.class.ctor = (*ZI224);
	
#line 1897 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-class */
			p_146 (flags, lex_state, act_state, err);
			p_150 (flags, lex_state, act_state, err, &ZIupper, &ZIend);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: mark-range */
			{
#line 535 "src/libre/parser.act"

		mark(&act_state->rangestart, &(*ZI225));
		mark(&act_state->rangeend,   &(ZIend));
	
#line 1913 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-range */
			/* BEGINNING OF ACTION: ast-make-class-range */
			{
#line 758 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		unsigned char lower, upper;

		AST_POS_OF_LX_POS(ast_start, (*ZI225));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		if ((ZIlower).type != AST_ENDPOINT_LITERAL ||
			(ZIupper).type != AST_ENDPOINT_LITERAL) {
			err->e = RE_EXUNSUPPORTD;
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

		(ZInode) = ast_make_class_range(&(ZIlower), ast_start, &(ZIupper), ast_end);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1951 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-class-range */
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

void
p_re__pcre(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 214 */
		{
			{
				p_expr (flags, lex_state, act_state, err, &ZInode);
				if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
					RESTORE_LEXER;
					goto ZL1;
				}
			}
		}
		/* END OF INLINE: 214 */
		/* BEGINNING OF INLINE: 215 */
		{
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_EOF):
					break;
				default:
					goto ZL4;
				}
				ADVANCE_LEXER;
			}
			goto ZL3;
		ZL4:;
			{
				/* BEGINNING OF ACTION: err-expected-eof */
				{
#line 519 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXEOF;
		}
		goto ZL1;
	
#line 2010 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL3:;
		}
		/* END OF INLINE: 215 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Catom(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__expr ZIe;

		/* BEGINNING OF INLINE: 186 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-make-expr-any */
					{
#line 653 "src/libre/parser.act"

		(ZIe) = ast_make_expr_any();
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 2052 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-expr-any */
				}
				break;
			case (TOK_CONTROL):
				{
					t_char ZI191;
					t_pos ZI192;
					t_pos ZI193;

					/* BEGINNING OF EXTRACT: CONTROL */
					{
#line 307 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] == 'c');
		assert(lex_state->buf.a[2] != '\0');
		assert(lex_state->buf.a[3] == '\0');

		ZI191 = lex_state->buf.a[2];
		if ((unsigned char) ZI191 > 127) {
			goto ZL1;
		}
		ZI191 = (((toupper((unsigned char)ZI191)) - 64) % 128 + 128) % 128;

		ZI192 = lex_state->lx.start;
		ZI193   = lex_state->lx.end;
	
#line 2081 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CONTROL */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: err-unsupported */
					{
#line 526 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXUNSUPPORTD;
		}
		goto ZL1;
	
#line 2094 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: err-unsupported */
					/* BEGINNING OF ACTION: ast-make-expr-empty */
					{
#line 625 "src/libre/parser.act"

		(ZIe) = ast_make_expr_empty();
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 2106 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-expr-empty */
				}
				break;
			case (TOK_END):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-make-expr-anchor-end */
					{
#line 718 "src/libre/parser.act"

		(ZIe) = ast_make_expr_anchor(AST_ANCHOR_END);
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 2123 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-expr-anchor-end */
				}
				break;
			case (TOK_OPENCAPTURE):
				{
					t_ast__expr ZIg;

					ADVANCE_LEXER;
					p_expr (flags, lex_state, act_state, err, &ZIg);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
					/* BEGINNING OF ACTION: ast-make-expr-group */
					{
#line 697 "src/libre/parser.act"

		(ZIe) = ast_make_expr_group((ZIg));
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 2147 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-expr-group */
					p_190 (flags, lex_state, act_state, err);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_OPENSUB):
				{
					ADVANCE_LEXER;
					p_expr (flags, lex_state, act_state, err, &ZIe);
					p_190 (flags, lex_state, act_state, err);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_START):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-make-expr-anchor-start */
					{
#line 711 "src/libre/parser.act"

		(ZIe) = ast_make_expr_anchor(AST_ANCHOR_START);
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 2180 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-expr-anchor-start */
				}
				break;
			case (TOK_OPENGROUP):
				{
					p_expr_C_Ccharacter_Hclass (flags, lex_state, act_state, err, &ZIe);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_OPENFLAGS):
				{
					p_expr_C_Cflags (flags, lex_state, act_state, err, &ZIe);
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
			case (TOK_NAMED__CLASS):
				{
					p_expr_C_Ctype (flags, lex_state, act_state, err, &ZIe);
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
		/* END OF INLINE: 186 */
		/* BEGINNING OF INLINE: 194 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_OPENCOUNT):
				{
					t_pos ZI248;
					t_pos ZI249;
					t_unsigned ZIm;

					/* BEGINNING OF EXTRACT: OPENCOUNT */
					{
#line 260 "src/libre/parser.act"

		ZI248 = lex_state->lx.start;
		ZI249   = lex_state->lx.end;
	
#line 2242 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OPENCOUNT */
					ADVANCE_LEXER;
					switch (CURRENT_TERMINAL) {
					case (TOK_COUNT):
						/* BEGINNING OF EXTRACT: COUNT */
						{
#line 416 "src/libre/parser.act"

		unsigned long u;
		char *e;

		u = strtoul(lex_state->buf.a, &e, 10);

		if ((u == ULONG_MAX && errno == ERANGE) || u > UINT_MAX) {
			err->e = RE_ECOUNTRANGE;
			snprintdots(err->esc, sizeof err->esc, lex_state->buf.a);
			goto ZL4;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXCOUNT;
			goto ZL4;
		}

		ZIm = (unsigned int) u;
	
#line 2270 "src/libre/dialect/pcre/parser.c"
						}
						/* END OF EXTRACT: COUNT */
						break;
					default:
						goto ZL4;
					}
					ADVANCE_LEXER;
					p_250 (flags, lex_state, act_state, err, &ZIe, &ZI248, &ZIm, &ZInode);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL4;
					}
				}
				break;
			case (TOK_OPT):
				{
					t_ast__count ZIc;

					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: atom-opt */
					{
#line 570 "src/libre/parser.act"

		(ZIc) = ast_make_count(0, NULL, 1, NULL);
	
#line 2296 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: atom-opt */
					/* BEGINNING OF ACTION: ast-make-expr-atom */
					{
#line 660 "src/libre/parser.act"

		(ZInode) = ast_make_expr_with_count((ZIe), (ZIc));
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL4;
		}
	
#line 2309 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-expr-atom */
				}
				break;
			case (TOK_PLUS):
				{
					t_ast__count ZIc;

					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: atom-plus */
					{
#line 562 "src/libre/parser.act"

		(ZIc) = ast_make_count(1, NULL, AST_COUNT_UNBOUNDED, NULL);
	
#line 2325 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: atom-plus */
					/* BEGINNING OF ACTION: ast-make-expr-atom */
					{
#line 660 "src/libre/parser.act"

		(ZInode) = ast_make_expr_with_count((ZIe), (ZIc));
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL4;
		}
	
#line 2338 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-expr-atom */
				}
				break;
			case (TOK_STAR):
				{
					t_ast__count ZIc;

					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: atom-kleene */
					{
#line 558 "src/libre/parser.act"

		(ZIc) = ast_make_count(0, NULL, AST_COUNT_UNBOUNDED, NULL);
	
#line 2354 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: atom-kleene */
					/* BEGINNING OF ACTION: ast-make-expr-atom */
					{
#line 660 "src/libre/parser.act"

		(ZInode) = ast_make_expr_with_count((ZIe), (ZIc));
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL4;
		}
	
#line 2367 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-expr-atom */
				}
				break;
			default:
				{
					t_ast__count ZIc;

					/* BEGINNING OF ACTION: atom-one */
					{
#line 566 "src/libre/parser.act"

		(ZIc) = ast_make_count(1, NULL, 1, NULL);
	
#line 2382 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: atom-one */
					/* BEGINNING OF ACTION: ast-make-expr-atom */
					{
#line 660 "src/libre/parser.act"

		(ZInode) = ast_make_expr_with_count((ZIe), (ZIc));
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL4;
		}
	
#line 2395 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-expr-atom */
				}
				break;
			}
			goto ZL3;
		ZL4:;
			{
				/* BEGINNING OF ACTION: err-expected-count */
				{
#line 463 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCOUNT;
		}
		goto ZL1;
	
#line 2413 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-count */
				ZInode = ZIe;
			}
		ZL3:;
		}
		/* END OF INLINE: 194 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_247(flags flags, lex_state lex_state, act_state act_state, err err, t_char *ZI244, t_pos *ZI245, t_ast__class *ZOnode)
{
	t_ast__class ZInode;

	switch (CURRENT_TERMINAL) {
	default:
		{
			/* BEGINNING OF ACTION: ast-make-class-literal */
			{
#line 748 "src/libre/parser.act"

		(ZInode) = ast_make_class_literal((*ZI244));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2447 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-class-literal */
		}
		break;
	case (TOK_RANGE):
		{
			t_endpoint ZIlower;
			t_endpoint ZIupper;
			t_pos ZIend;

			/* BEGINNING OF ACTION: ast-range-endpoint-literal */
			{
#line 594 "src/libre/parser.act"

		(ZIlower).type = AST_ENDPOINT_LITERAL;
		(ZIlower).u.literal.c = (*ZI244);
	
#line 2465 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
			p_146 (flags, lex_state, act_state, err);
			p_150 (flags, lex_state, act_state, err, &ZIupper, &ZIend);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: mark-range */
			{
#line 535 "src/libre/parser.act"

		mark(&act_state->rangestart, &(*ZI245));
		mark(&act_state->rangeend,   &(ZIend));
	
#line 2481 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-range */
			/* BEGINNING OF ACTION: ast-make-class-range */
			{
#line 758 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		unsigned char lower, upper;

		AST_POS_OF_LX_POS(ast_start, (*ZI245));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		if ((ZIlower).type != AST_ENDPOINT_LITERAL ||
			(ZIupper).type != AST_ENDPOINT_LITERAL) {
			err->e = RE_EXUNSUPPORTD;
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

		(ZInode) = ast_make_class_range(&(ZIlower), ast_start, &(ZIupper), ast_end);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2519 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-class-range */
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

	switch (CURRENT_TERMINAL) {
	case (TOK_ANY): case (TOK_START): case (TOK_END): case (TOK_OPENSUB):
	case (TOK_OPENCAPTURE): case (TOK_OPENGROUP): case (TOK_NAMED__CLASS): case (TOK_OPENFLAGS):
	case (TOK_ESC): case (TOK_CONTROL): case (TOK_OCT): case (TOK_HEX):
	case (TOK_CHAR):
		{
			/* BEGINNING OF ACTION: ast-make-expr-concat */
			{
#line 632 "src/libre/parser.act"

		(ZInode) = ast_make_expr_concat();
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2555 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-expr-concat */
			p_expr_C_Clist_Hof_Hatoms (flags, lex_state, act_state, err, ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: ast-make-expr-empty */
			{
#line 625 "src/libre/parser.act"

		(ZInode) = ast_make_expr_empty();
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2576 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-expr-empty */
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
p_250(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZIe, t_pos *ZI248, t_unsigned *ZIm, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_CLOSECOUNT):
		{
			t_pos ZI197;
			t_pos ZIend;
			t_ast__count ZIc;

			/* BEGINNING OF EXTRACT: CLOSECOUNT */
			{
#line 265 "src/libre/parser.act"

		ZI197 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 2611 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: CLOSECOUNT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: mark-count */
			{
#line 540 "src/libre/parser.act"

		mark(&act_state->countstart, &(*ZI248));
		mark(&act_state->countend,   &(ZIend));
	
#line 2622 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-count */
			/* BEGINNING OF ACTION: atom-count */
			{
#line 576 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;

		if ((*ZIm) < (*ZIm)) {
			err->e = RE_ENEGCOUNT;
			err->m = (*ZIm);
			err->n = (*ZIm);

			mark(&act_state->countstart, &(*ZI248));
			mark(&act_state->countend,   &(ZIend));

			goto ZL1;
		}

		AST_POS_OF_LX_POS(ast_start, (*ZI248));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		(ZIc) = ast_make_count((*ZIm), &ast_start, (*ZIm), &ast_end);
	
#line 2647 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: atom-count */
			/* BEGINNING OF ACTION: ast-make-expr-atom */
			{
#line 660 "src/libre/parser.act"

		(ZInode) = ast_make_expr_with_count((*ZIe), (ZIc));
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL1;
		}
	
#line 2660 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-expr-atom */
		}
		break;
	case (TOK_SEP):
		{
			t_unsigned ZIn;
			t_pos ZIend;
			t_pos ZI200;
			t_ast__count ZIc;

			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (TOK_COUNT):
				/* BEGINNING OF EXTRACT: COUNT */
				{
#line 416 "src/libre/parser.act"

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
	
#line 2697 "src/libre/dialect/pcre/parser.c"
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
#line 265 "src/libre/parser.act"

		ZIend = lex_state->lx.start;
		ZI200   = lex_state->lx.end;
	
#line 2714 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF EXTRACT: CLOSECOUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: mark-count */
			{
#line 540 "src/libre/parser.act"

		mark(&act_state->countstart, &(*ZI248));
		mark(&act_state->countend,   &(ZIend));
	
#line 2729 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-count */
			/* BEGINNING OF ACTION: atom-count */
			{
#line 576 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;

		if ((ZIn) < (*ZIm)) {
			err->e = RE_ENEGCOUNT;
			err->m = (*ZIm);
			err->n = (ZIn);

			mark(&act_state->countstart, &(*ZI248));
			mark(&act_state->countend,   &(ZIend));

			goto ZL1;
		}

		AST_POS_OF_LX_POS(ast_start, (*ZI248));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		(ZIc) = ast_make_count((*ZIm), &ast_start, (ZIn), &ast_end);
	
#line 2754 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: atom-count */
			/* BEGINNING OF ACTION: ast-make-expr-atom */
			{
#line 660 "src/libre/parser.act"

		(ZInode) = ast_make_expr_with_count((*ZIe), (ZIc));
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL1;
		}
	
#line 2767 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-expr-atom */
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
p_expr_C_Ctype(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__class__id ZIid;
		t_pos ZIstart;
		t_pos ZIend;
		t_ast__class ZInamed;

		switch (CURRENT_TERMINAL) {
		case (TOK_NAMED__CLASS):
			/* BEGINNING OF EXTRACT: NAMED_CLASS */
			{
#line 428 "src/libre/parser.act"

		ZIid = DIALECT_CLASS(lex_state->buf.a);
		if (ZIid == NULL) {
			/* syntax error -- unrecognized class */
			goto ZL1;
		}

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 2814 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: NAMED_CLASS */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: ast-make-class-named */
		{
#line 788 "src/libre/parser.act"

		(ZInamed) = ast_make_class_named((ZIid));
		if ((ZInamed) == NULL) {
			goto ZL1;
		}
	
#line 2831 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-make-class-named */
		/* BEGINNING OF ACTION: ast-make-expr-class */
		{
#line 684 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;

		AST_POS_OF_LX_POS(ast_start, (ZIstart));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));

		(ZInode) = ast_make_expr_class((ZInamed), &ast_start, &ast_end);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2851 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-make-expr-class */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

/* BEGINNING OF TRAILER */

#line 977 "src/libre/parser.act"


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
		const struct fsm_options *opt,
		enum re_flags flags, int overlap,
		struct re_err *err)
	{
		struct ast *ast;
		struct flags top, *fl = &top;

		struct act_state  act_state_s;
		struct act_state *act_state;
		struct lex_state  lex_state_s;
		struct lex_state *lex_state;
		struct re_err dummy;

		struct LX_STATE *lx;

		top.flags = flags;

		assert(f != NULL);

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

		err->e = RE_ESUCCESS;

		ADVANCE_LEXER;
		DIALECT_ENTRY(fl, lex_state, act_state, err, &ast->expr);

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

#line 3008 "src/libre/dialect/pcre/parser.c"

/* END OF FILE */
