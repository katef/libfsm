/*
 * Automatically generated from the files:
 *	src/libre/dialect/pcre/parser.sid
 * and
 *	src/libre/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 22 "src/libre/parser.act"


	#include <assert.h>
	#include <limits.h>
	#include <string.h>
	#include <stdint.h>
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

	typedef char     t_char;
	typedef unsigned t_unsigned;
	typedef unsigned t_pred; /* TODO */

	typedef struct lx_pos t_pos;
	typedef enum re_flags t_re__flags;
	typedef const struct class * t_ast__class__id;
	typedef struct ast_count t_ast__count;
	typedef struct ast_endpoint t_endpoint;

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

#line 210 "src/libre/dialect/pcre/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_expr_C_Cflags_C_Cflag__set(flags, lex_state, act_state, err, t_re__flags, t_re__flags *);
static void p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_C_Crange_Hendpoint_Hliteral(flags, lex_state, act_state, err, t_endpoint *, t_pos *, t_pos *);
static void p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms(flags, lex_state, act_state, err, t_ast__expr);
static void p_272(flags, lex_state, act_state, err, t_ast__class__id *, t_pos *, t_ast__expr *);
static void p_153(flags, lex_state, act_state, err);
static void p_292(flags, lex_state, act_state, err, t_char *, t_pos *, t_ast__expr *);
static void p_expr_C_Clist_Hof_Hpieces(flags, lex_state, act_state, err, t_ast__expr);
static void p_295(flags, lex_state, act_state, err, t_pos *, t_unsigned *, t_ast__count *);
static void p_296(flags, lex_state, act_state, err, t_pos *, t_unsigned *, t_ast__count *);
static void p_expr_C_Cliteral(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Ccharacter_Hclass_C_Cclass_Hterm(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Ccomment(flags, lex_state, act_state, err);
static void p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_C_Crange_Hendpoint_Hclass(flags, lex_state, act_state, err, t_endpoint *, t_pos *, t_pos *);
static void p_expr_C_Ccharacter_Hclass(flags, lex_state, act_state, err, t_ast__expr *);
static void p_179(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_Hend(flags, lex_state, act_state, err, t_endpoint *, t_pos *);
static void p_expr_C_Cpiece(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr(flags, lex_state, act_state, err, t_ast__expr *);
static void p_194(flags, lex_state, act_state, err, t_pos *, t_char *, t_ast__expr *);
static void p_expr_C_Cflags(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Cpiece_C_Clist_Hof_Hcounts(flags, lex_state, act_state, err, t_ast__expr, t_ast__expr *);
static void p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint(flags, lex_state, act_state, err, t_endpoint *, t_pos *, t_pos *);
static void p_class_Hnamed(flags, lex_state, act_state, err, t_ast__expr *, t_pos *, t_pos *);
static void p_expr_C_Clist_Hof_Halts(flags, lex_state, act_state, err, t_ast__expr);
static void p_expr_C_Cpiece_C_Ccount(flags, lex_state, act_state, err, t_ast__count *);
extern void p_re__pcre(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Cpiece_C_Catom(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Calt(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Ctype(flags, lex_state, act_state, err, t_ast__expr *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

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
#line 492 "src/libre/parser.act"

		ZIc = RE_ICASE;
	
#line 270 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: FLAG_INSENSITIVE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: re-flag-union */
			{
#line 634 "src/libre/parser.act"

		(ZIo) = (ZIi) | (ZIc);
	
#line 280 "src/libre/dialect/pcre/parser.c"
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
#line 562 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EFLAG;
		}
		goto ZL1;
	
#line 298 "src/libre/dialect/pcre/parser.c"
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

		/* BEGINNING OF INLINE: 140 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 451 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		ZIc = lex_state->buf.a[0];
	
#line 346 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_CONTROL):
				{
					/* BEGINNING OF EXTRACT: CONTROL */
					{
#line 332 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] == 'c');
		assert(lex_state->buf.a[2] != '\0');
		assert(lex_state->buf.a[3] == '\0');

		ZIc = lex_state->buf.a[2];
		/* from http://www.pcre.org/current/doc/html/pcre2pattern.html#SEC5
		 * 
		 *  The precise effect of \cx on ASCII characters is as follows: if x is a lower case letter, it is converted to
		 *  upper case. Then bit 6 of the character (hex 40) is inverted. Thus \cA to \cZ become hex 01 to hex 1A (A is
		 *  41, Z is 5A), but \c{ becomes hex 3B ({ is 7B), and \c; becomes hex 7B (; is 3B). If the code unit following
		 *  \c has a value less than 32 or greater than 126, a compile-time error occurs. 
		 */

		{
			unsigned char cc = (unsigned char)ZIc;
			if (cc > 126 || cc < 32) {
				goto ZL1;
			}
			
			if (islower(cc)) {
				cc = toupper(cc);
			}

			cc ^= 0x40;

			ZIc = cc;
		}

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 390 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CONTROL */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: err-unsupported */
					{
#line 583 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXUNSUPPORTD;
		}
		goto ZL1;
	
#line 403 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: err-unsupported */
				}
				break;
			case (TOK_ESC):
				{
					/* BEGINNING OF EXTRACT: ESC */
					{
#line 285 "src/libre/parser.act"

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
	
#line 435 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: ESC */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_HEX):
				{
					/* BEGINNING OF EXTRACT: HEX */
					{
#line 406 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 2);  /* pcre allows \x without a suffix */

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

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
	
#line 490 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: HEX */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_OCT):
				{
					/* BEGINNING OF EXTRACT: OCT */
					{
#line 366 "src/libre/parser.act"

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
	
#line 540 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OCT */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 140 */
		/* BEGINNING OF ACTION: ast-range-endpoint-literal */
		{
#line 678 "src/libre/parser.act"

		(ZIr).type = AST_ENDPOINT_LITERAL;
		(ZIr).u.literal.c = (ZIc);
	
#line 558 "src/libre/dialect/pcre/parser.c"
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
p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr ZIclass)
{
ZL2_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms:;
	switch (CURRENT_TERMINAL) {
	case (TOK_NAMED__CLASS): case (TOK_ESC): case (TOK_NOESC): case (TOK_CONTROL):
	case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
		{
			t_ast__expr ZInode;

			p_expr_C_Ccharacter_Hclass_C_Cclass_Hterm (flags, lex_state, act_state, err, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: ast-add-alt */
			{
#line 867 "src/libre/parser.act"

		if (!ast_add_expr_alt((ZIclass), (ZInode))) {
			goto ZL1;
		}
	
#line 595 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-add-alt */
			/* BEGINNING OF INLINE: expr::character-class::list-of-class-terms */
			goto ZL2_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms;
			/* END OF INLINE: expr::character-class::list-of-class-terms */
		}
		/* UNREACHED */
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
p_272(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__class__id *ZI269, t_pos *ZI270, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	default:
		{
			/* BEGINNING OF ACTION: ast-make-named */
			{
#line 854 "src/libre/parser.act"

		(ZInode) = ast_make_expr_named(act_state->poolp, *flags, (*ZI269));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 631 "src/libre/dialect/pcre/parser.c"
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
#line 683 "src/libre/parser.act"

		(ZIlower).type = AST_ENDPOINT_NAMED;
		(ZIlower).u.named.class = (*ZI269);
	
#line 649 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-class */
			p_153 (flags, lex_state, act_state, err);
			p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_Hend (flags, lex_state, act_state, err, &ZIupper, &ZIend);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: mark-range */
			{
#line 595 "src/libre/parser.act"

		mark(&act_state->rangestart, &(*ZI270));
		mark(&act_state->rangeend,   &(ZIend));
	
#line 665 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-range */
			/* BEGINNING OF ACTION: ast-make-range */
			{
#line 821 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		unsigned char lower, upper;

		AST_POS_OF_LX_POS(ast_start, (*ZI270));
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

		(ZInode) = ast_make_expr_range(act_state->poolp, *flags, &(ZIlower), ast_start, &(ZIupper), ast_end);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 703 "src/libre/dialect/pcre/parser.c"
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
p_153(flags flags, lex_state lex_state, act_state act_state, err err)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZI154;
		t_pos ZI155;
		t_pos ZI156;

		switch (CURRENT_TERMINAL) {
		case (TOK_RANGE):
			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 237 "src/libre/parser.act"

		ZI154 = '-';
		ZI155 = lex_state->lx.start;
		ZI156   = lex_state->lx.end;
	
#line 740 "src/libre/dialect/pcre/parser.c"
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
#line 541 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXRANGE;
		}
		goto ZL2;
	
#line 761 "src/libre/dialect/pcre/parser.c"
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
p_292(flags flags, lex_state lex_state, act_state act_state, err err, t_char *ZI289, t_pos *ZI290, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	default:
		{
			/* BEGINNING OF ACTION: ast-make-literal */
			{
#line 713 "src/libre/parser.act"

		(ZInode) = ast_make_expr_literal(act_state->poolp, *flags, (*ZI289));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 789 "src/libre/dialect/pcre/parser.c"
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
#line 678 "src/libre/parser.act"

		(ZIlower).type = AST_ENDPOINT_LITERAL;
		(ZIlower).u.literal.c = (*ZI289);
	
#line 807 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
			p_153 (flags, lex_state, act_state, err);
			p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_Hend (flags, lex_state, act_state, err, &ZIupper, &ZIend);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: mark-range */
			{
#line 595 "src/libre/parser.act"

		mark(&act_state->rangestart, &(*ZI290));
		mark(&act_state->rangeend,   &(ZIend));
	
#line 823 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-range */
			/* BEGINNING OF ACTION: ast-make-range */
			{
#line 821 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		unsigned char lower, upper;

		AST_POS_OF_LX_POS(ast_start, (*ZI290));
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

		(ZInode) = ast_make_expr_range(act_state->poolp, *flags, &(ZIlower), ast_start, &(ZIupper), ast_end);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 861 "src/libre/dialect/pcre/parser.c"
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
p_expr_C_Clist_Hof_Hpieces(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr ZIcat)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_expr_C_Clist_Hof_Hpieces:;
	{
		/* BEGINNING OF INLINE: 249 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_INVALID__COMMENT):
				{
					p_expr_C_Ccomment (flags, lex_state, act_state, err);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_ANY): case (TOK_START): case (TOK_END): case (TOK_OPEN):
			case (TOK_OPENGROUP): case (TOK_OPENGROUPINV): case (TOK_OPENGROUPCB): case (TOK_OPENGROUPINVCB):
			case (TOK_NAMED__CLASS): case (TOK_FLAGS): case (TOK_ESC): case (TOK_NOESC):
			case (TOK_CONTROL): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
			case (TOK_UNSUPPORTED):
				{
					t_ast__expr ZIa;

					p_expr_C_Cpiece (flags, lex_state, act_state, err, &ZIa);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
					/* BEGINNING OF ACTION: ast-add-concat */
					{
#line 861 "src/libre/parser.act"

		if (!ast_add_expr_concat((ZIcat), (ZIa))) {
			goto ZL1;
		}
	
#line 918 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-add-concat */
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 249 */
		/* BEGINNING OF INLINE: 250 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY): case (TOK_START): case (TOK_END): case (TOK_OPEN):
			case (TOK_OPENGROUP): case (TOK_OPENGROUPINV): case (TOK_OPENGROUPCB): case (TOK_OPENGROUPINVCB):
			case (TOK_NAMED__CLASS): case (TOK_FLAGS): case (TOK_ESC): case (TOK_NOESC):
			case (TOK_CONTROL): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
			case (TOK_UNSUPPORTED): case (TOK_INVALID__COMMENT):
				{
					/* BEGINNING OF INLINE: expr::list-of-pieces */
					goto ZL2_expr_C_Clist_Hof_Hpieces;
					/* END OF INLINE: expr::list-of-pieces */
				}
				/* UNREACHED */
			default:
				break;
			}
		}
		/* END OF INLINE: 250 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_295(flags flags, lex_state lex_state, act_state act_state, err err, t_pos *ZI293, t_unsigned *ZIm, t_ast__count *ZOc)
{
	t_ast__count ZIc;

	switch (CURRENT_TERMINAL) {
	case (TOK_CLOSECOUNT):
		{
			t_pos ZI234;
			t_pos ZIend;

			/* BEGINNING OF EXTRACT: CLOSECOUNT */
			{
#line 280 "src/libre/parser.act"

		ZI234 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 972 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: CLOSECOUNT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: mark-count */
			{
#line 600 "src/libre/parser.act"

		mark(&act_state->countstart, &(*ZI293));
		mark(&act_state->countend,   &(ZIend));
	
#line 983 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-count */
			/* BEGINNING OF ACTION: count-range */
			{
#line 658 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;

		if ((*ZIm) < (*ZIm)) {
			err->e = RE_ENEGCOUNT;
			err->m = (*ZIm);
			err->n = (*ZIm);

			mark(&act_state->countstart, &(*ZI293));
			mark(&act_state->countend,   &(ZIend));

			goto ZL1;
		}

		AST_POS_OF_LX_POS(ast_start, (*ZI293));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		(ZIc) = ast_make_count((*ZIm), &ast_start, (*ZIm), &ast_end);
	
#line 1008 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: count-range */
		}
		break;
	case (TOK_SEP):
		{
			ADVANCE_LEXER;
			p_296 (flags, lex_state, act_state, err, ZI293, ZIm, &ZIc);
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
	*ZOc = ZIc;
}

static void
p_296(flags flags, lex_state lex_state, act_state act_state, err err, t_pos *ZI293, t_unsigned *ZIm, t_ast__count *ZOc)
{
	t_ast__count ZIc;

	switch (CURRENT_TERMINAL) {
	case (TOK_CLOSECOUNT):
		{
			t_pos ZI239;
			t_pos ZIend;
			t_unsigned ZIn;

			/* BEGINNING OF EXTRACT: CLOSECOUNT */
			{
#line 280 "src/libre/parser.act"

		ZI239 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 1055 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: CLOSECOUNT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: mark-count */
			{
#line 600 "src/libre/parser.act"

		mark(&act_state->countstart, &(*ZI293));
		mark(&act_state->countend,   &(ZIend));
	
#line 1066 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-count */
			/* BEGINNING OF ACTION: count-unbounded */
			{
#line 638 "src/libre/parser.act"

		(ZIn) = AST_COUNT_UNBOUNDED;
	
#line 1075 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: count-unbounded */
			/* BEGINNING OF ACTION: count-range */
			{
#line 658 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;

		if ((ZIn) < (*ZIm)) {
			err->e = RE_ENEGCOUNT;
			err->m = (*ZIm);
			err->n = (ZIn);

			mark(&act_state->countstart, &(*ZI293));
			mark(&act_state->countend,   &(ZIend));

			goto ZL1;
		}

		AST_POS_OF_LX_POS(ast_start, (*ZI293));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		(ZIc) = ast_make_count((*ZIm), &ast_start, (ZIn), &ast_end);
	
#line 1100 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: count-range */
		}
		break;
	case (TOK_COUNT):
		{
			t_unsigned ZIn;
			t_pos ZI237;
			t_pos ZIend;

			/* BEGINNING OF EXTRACT: COUNT */
			{
#line 461 "src/libre/parser.act"

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
	
#line 1133 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: COUNT */
			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSECOUNT):
				/* BEGINNING OF EXTRACT: CLOSECOUNT */
				{
#line 280 "src/libre/parser.act"

		ZI237 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 1146 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF EXTRACT: CLOSECOUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: mark-count */
			{
#line 600 "src/libre/parser.act"

		mark(&act_state->countstart, &(*ZI293));
		mark(&act_state->countend,   &(ZIend));
	
#line 1161 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-count */
			/* BEGINNING OF ACTION: count-range */
			{
#line 658 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;

		if ((ZIn) < (*ZIm)) {
			err->e = RE_ENEGCOUNT;
			err->m = (*ZIm);
			err->n = (ZIn);

			mark(&act_state->countstart, &(*ZI293));
			mark(&act_state->countend,   &(ZIend));

			goto ZL1;
		}

		AST_POS_OF_LX_POS(ast_start, (*ZI293));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		(ZIc) = ast_make_count((*ZIm), &ast_start, (ZIn), &ast_end);
	
#line 1186 "src/libre/dialect/pcre/parser.c"
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
p_expr_C_Cliteral(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZIc;

		/* BEGINNING OF INLINE: 98 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					t_pos ZI108;
					t_pos ZI109;

					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 451 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI108 = lex_state->lx.start;
		ZI109   = lex_state->lx.end;

		ZIc = lex_state->buf.a[0];
	
#line 1235 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_CONTROL):
				{
					t_pos ZI110;
					t_pos ZI111;

					/* BEGINNING OF EXTRACT: CONTROL */
					{
#line 332 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] == 'c');
		assert(lex_state->buf.a[2] != '\0');
		assert(lex_state->buf.a[3] == '\0');

		ZIc = lex_state->buf.a[2];
		/* from http://www.pcre.org/current/doc/html/pcre2pattern.html#SEC5
		 * 
		 *  The precise effect of \cx on ASCII characters is as follows: if x is a lower case letter, it is converted to
		 *  upper case. Then bit 6 of the character (hex 40) is inverted. Thus \cA to \cZ become hex 01 to hex 1A (A is
		 *  41, Z is 5A), but \c{ becomes hex 3B ({ is 7B), and \c; becomes hex 7B (; is 3B). If the code unit following
		 *  \c has a value less than 32 or greater than 126, a compile-time error occurs. 
		 */

		{
			unsigned char cc = (unsigned char)ZIc;
			if (cc > 126 || cc < 32) {
				goto ZL1;
			}
			
			if (islower(cc)) {
				cc = toupper(cc);
			}

			cc ^= 0x40;

			ZIc = cc;
		}

		ZI110 = lex_state->lx.start;
		ZI111   = lex_state->lx.end;
	
#line 1282 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CONTROL */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_ESC):
				{
					t_pos ZI100;
					t_pos ZI101;

					/* BEGINNING OF EXTRACT: ESC */
					{
#line 285 "src/libre/parser.act"

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

		ZI100 = lex_state->lx.start;
		ZI101   = lex_state->lx.end;
	
#line 1318 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: ESC */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_HEX):
				{
					t_pos ZI106;
					t_pos ZI107;

					/* BEGINNING OF EXTRACT: HEX */
					{
#line 406 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 2);  /* pcre allows \x without a suffix */

		ZI106 = lex_state->lx.start;
		ZI107   = lex_state->lx.end;

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
	
#line 1376 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: HEX */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_NOESC):
				{
					t_pos ZI102;
					t_pos ZI103;

					/* BEGINNING OF EXTRACT: NOESC */
					{
#line 308 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZIc = lex_state->buf.a[1];

		ZI102 = lex_state->lx.start;
		ZI103   = lex_state->lx.end;
	
#line 1400 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: NOESC */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_OCT):
				{
					t_pos ZI104;
					t_pos ZI105;

					/* BEGINNING OF EXTRACT: OCT */
					{
#line 366 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		ZI104 = lex_state->lx.start;
		ZI105   = lex_state->lx.end;

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
	
#line 1453 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OCT */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_UNSUPPORTED):
				{
					t_pos ZI112;
					t_pos ZI113;

					/* BEGINNING OF EXTRACT: UNSUPPORTED */
					{
#line 319 "src/libre/parser.act"

		/* handle \1-\9 back references */
		if (lex_state->buf.a[0] == '\\' && lex_state->buf.a[1] != '\0' && lex_state->buf.a[2] == '\0') {
			ZIc = lex_state->buf.a[1];
		}
		else {
			ZIc = lex_state->buf.a[0];
		}

		ZI112 = lex_state->lx.start;
		ZI113   = lex_state->lx.end;
	
#line 1479 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: UNSUPPORTED */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: err-unsupported */
					{
#line 583 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXUNSUPPORTD;
		}
		goto ZL1;
	
#line 1492 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: err-unsupported */
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 98 */
		/* BEGINNING OF ACTION: ast-make-literal */
		{
#line 713 "src/libre/parser.act"

		(ZInode) = ast_make_expr_literal(act_state->poolp, *flags, (ZIc));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1511 "src/libre/dialect/pcre/parser.c"
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
			t_char ZI285;
			t_pos ZI286;
			t_pos ZI287;

			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 451 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI286 = lex_state->lx.start;
		ZI287   = lex_state->lx.end;

		ZI285 = lex_state->buf.a[0];
	
#line 1547 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			ADVANCE_LEXER;
			p_292 (flags, lex_state, act_state, err, &ZI285, &ZI286, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_CONTROL):
		{
			t_char ZI289;
			t_pos ZI290;
			t_pos ZI291;

			/* BEGINNING OF EXTRACT: CONTROL */
			{
#line 332 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] == 'c');
		assert(lex_state->buf.a[2] != '\0');
		assert(lex_state->buf.a[3] == '\0');

		ZI289 = lex_state->buf.a[2];
		/* from http://www.pcre.org/current/doc/html/pcre2pattern.html#SEC5
		 * 
		 *  The precise effect of \cx on ASCII characters is as follows: if x is a lower case letter, it is converted to
		 *  upper case. Then bit 6 of the character (hex 40) is inverted. Thus \cA to \cZ become hex 01 to hex 1A (A is
		 *  41, Z is 5A), but \c{ becomes hex 3B ({ is 7B), and \c; becomes hex 7B (; is 3B). If the code unit following
		 *  \c has a value less than 32 or greater than 126, a compile-time error occurs. 
		 */

		{
			unsigned char cc = (unsigned char)ZI289;
			if (cc > 126 || cc < 32) {
				goto ZL1;
			}
			
			if (islower(cc)) {
				cc = toupper(cc);
			}

			cc ^= 0x40;

			ZI289 = cc;
		}

		ZI290 = lex_state->lx.start;
		ZI291   = lex_state->lx.end;
	
#line 1600 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: CONTROL */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: err-unsupported */
			{
#line 583 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXUNSUPPORTD;
		}
		goto ZL1;
	
#line 1613 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: err-unsupported */
			p_292 (flags, lex_state, act_state, err, &ZI289, &ZI290, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_ESC):
		{
			t_char ZI273;
			t_pos ZI274;
			t_pos ZI275;

			/* BEGINNING OF EXTRACT: ESC */
			{
#line 285 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZI273 = lex_state->buf.a[1];

		switch (ZI273) {
		case 'a': ZI273 = '\a'; break;
		case 'b': ZI273 = '\b'; break;
		case 'e': ZI273 = '\033'; break;
		case 'f': ZI273 = '\f'; break;
		case 'n': ZI273 = '\n'; break;
		case 'r': ZI273 = '\r'; break;
		case 't': ZI273 = '\t'; break;
		case 'v': ZI273 = '\v'; break;
		default:             break;
		}

		ZI274 = lex_state->lx.start;
		ZI275   = lex_state->lx.end;
	
#line 1654 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: ESC */
			ADVANCE_LEXER;
			p_292 (flags, lex_state, act_state, err, &ZI273, &ZI274, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_HEX):
		{
			t_char ZI281;
			t_pos ZI282;
			t_pos ZI283;

			/* BEGINNING OF EXTRACT: HEX */
			{
#line 406 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 2);  /* pcre allows \x without a suffix */

		ZI282 = lex_state->lx.start;
		ZI283   = lex_state->lx.end;

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

		ZI281 = (char) (unsigned char) u;
	
#line 1718 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: HEX */
			ADVANCE_LEXER;
			p_292 (flags, lex_state, act_state, err, &ZI281, &ZI282, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_NAMED__CLASS):
		{
			t_ast__class__id ZI269;
			t_pos ZI270;
			t_pos ZI271;

			/* BEGINNING OF EXTRACT: NAMED_CLASS */
			{
#line 481 "src/libre/parser.act"

		ZI269 = DIALECT_CLASS(lex_state->buf.a);
		if (ZI269 == NULL) {
			/* syntax error -- unrecognized class */
			goto ZL1;
		}

		ZI270 = lex_state->lx.start;
		ZI271   = lex_state->lx.end;
	
#line 1748 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: NAMED_CLASS */
			ADVANCE_LEXER;
			p_272 (flags, lex_state, act_state, err, &ZI269, &ZI270, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_NOESC):
		{
			t_char ZIc;
			t_pos ZI121;
			t_pos ZI122;

			/* BEGINNING OF EXTRACT: NOESC */
			{
#line 308 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZIc = lex_state->buf.a[1];

		ZI121 = lex_state->lx.start;
		ZI122   = lex_state->lx.end;
	
#line 1778 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: NOESC */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-make-literal */
			{
#line 713 "src/libre/parser.act"

		(ZInode) = ast_make_expr_literal(act_state->poolp, *flags, (ZIc));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1791 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-literal */
		}
		break;
	case (TOK_OCT):
		{
			t_char ZI277;
			t_pos ZI278;
			t_pos ZI279;

			/* BEGINNING OF EXTRACT: OCT */
			{
#line 366 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		ZI278 = lex_state->lx.start;
		ZI279   = lex_state->lx.end;

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

		ZI277 = (char) (unsigned char) u;
	
#line 1844 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: OCT */
			ADVANCE_LEXER;
			p_292 (flags, lex_state, act_state, err, &ZI277, &ZI278, &ZInode);
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
p_expr_C_Ccomment(flags flags, lex_state lex_state, act_state act_state, err err)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		switch (CURRENT_TERMINAL) {
		case (TOK_INVALID__COMMENT):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: err-invalid-comment */
		{
#line 506 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EBADCOMMENT;
		}
		goto ZL1;
	
#line 1891 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: err-invalid-comment */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
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
#line 481 "src/libre/parser.act"

		ZIid = DIALECT_CLASS(lex_state->buf.a);
		if (ZIid == NULL) {
			/* syntax error -- unrecognized class */
			goto ZL1;
		}

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 1929 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: NAMED_CLASS */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: ast-range-endpoint-class */
		{
#line 683 "src/libre/parser.act"

		(ZIr).type = AST_ENDPOINT_NAMED;
		(ZIr).u.named.class = (ZIid);
	
#line 1944 "src/libre/dialect/pcre/parser.c"
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
		t_pos ZIend;

		/* BEGINNING OF INLINE: 165 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_OPENGROUP):
				{
					t_pos ZI166;

					/* BEGINNING OF EXTRACT: OPENGROUP */
					{
#line 243 "src/libre/parser.act"

		ZIstart = lex_state->lx.start;
		ZI166   = lex_state->lx.end;
	
#line 1985 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OPENGROUP */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-make-alt */
					{
#line 706 "src/libre/parser.act"

		(ZInode) = ast_make_expr_alt(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1998 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-alt */
					ZItmp = ZInode;
					p_179 (flags, lex_state, act_state, err, &ZItmp);
					p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms (flags, lex_state, act_state, err, ZItmp);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_OPENGROUPCB):
				{
					t_pos ZI185;
					t_char ZIcbrak;
					t_ast__expr ZInode1;

					/* BEGINNING OF EXTRACT: OPENGROUPCB */
					{
#line 253 "src/libre/parser.act"

		ZIstart = lex_state->lx.start;
		ZI185   = lex_state->lx.end;
	
#line 2023 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OPENGROUPCB */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-make-alt */
					{
#line 706 "src/libre/parser.act"

		(ZInode) = ast_make_expr_alt(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2036 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-alt */
					ZItmp = ZInode;
					/* BEGINNING OF ACTION: make-literal-cbrak */
					{
#line 720 "src/libre/parser.act"

		(ZIcbrak) = ']';
	
#line 2046 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: make-literal-cbrak */
					p_194 (flags, lex_state, act_state, err, &ZIstart, &ZIcbrak, &ZInode1);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
					/* BEGINNING OF ACTION: ast-add-alt */
					{
#line 867 "src/libre/parser.act"

		if (!ast_add_expr_alt((ZItmp), (ZInode1))) {
			goto ZL1;
		}
	
#line 2062 "src/libre/dialect/pcre/parser.c"
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
					t_pos ZI177;

					/* BEGINNING OF EXTRACT: OPENGROUPINV */
					{
#line 248 "src/libre/parser.act"

		ZIstart = lex_state->lx.start;
		ZI177   = lex_state->lx.end;
	
#line 2083 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OPENGROUPINV */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-make-alt */
					{
#line 706 "src/libre/parser.act"

		(ZInode) = ast_make_expr_alt(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2096 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-alt */
					ZItmp = ZInode;
					/* BEGINNING OF ACTION: ast-make-invert */
					{
#line 783 "src/libre/parser.act"

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
	
#line 2140 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-invert */
					p_179 (flags, lex_state, act_state, err, &ZItmp);
					p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms (flags, lex_state, act_state, err, ZItmp);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_OPENGROUPINVCB):
				{
					t_pos ZI192;
					t_char ZIcbrak;
					t_ast__expr ZInode1;

					/* BEGINNING OF EXTRACT: OPENGROUPINVCB */
					{
#line 258 "src/libre/parser.act"

		ZIstart = lex_state->lx.start;
		ZI192   = lex_state->lx.end;
	
#line 2164 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OPENGROUPINVCB */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-make-alt */
					{
#line 706 "src/libre/parser.act"

		(ZInode) = ast_make_expr_alt(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2177 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-alt */
					ZItmp = ZInode;
					/* BEGINNING OF ACTION: ast-make-invert */
					{
#line 783 "src/libre/parser.act"

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
	
#line 2221 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-invert */
					/* BEGINNING OF ACTION: make-literal-cbrak */
					{
#line 720 "src/libre/parser.act"

		(ZIcbrak) = ']';
	
#line 2230 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: make-literal-cbrak */
					p_194 (flags, lex_state, act_state, err, &ZIstart, &ZIcbrak, &ZInode1);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
					/* BEGINNING OF ACTION: ast-add-alt */
					{
#line 867 "src/libre/parser.act"

		if (!ast_add_expr_alt((ZItmp), (ZInode1))) {
			goto ZL1;
		}
	
#line 2246 "src/libre/dialect/pcre/parser.c"
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
		/* END OF INLINE: 165 */
		/* BEGINNING OF INLINE: 198 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSEGROUP):
				{
					t_char ZI199;
					t_pos ZI200;

					/* BEGINNING OF EXTRACT: CLOSEGROUP */
					{
#line 263 "src/libre/parser.act"

		ZI199 = ']';
		ZI200 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 2277 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CLOSEGROUP */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: mark-group */
					{
#line 590 "src/libre/parser.act"

		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));
	
#line 2288 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: mark-group */
				}
				break;
			case (TOK_CLOSEGROUPRANGE):
				{
					t_char ZIcrange;
					t_pos ZI202;
					t_ast__expr ZIrange;

					/* BEGINNING OF EXTRACT: CLOSEGROUPRANGE */
					{
#line 269 "src/libre/parser.act"

		ZIcrange = '-';
		ZI202 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 2307 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CLOSEGROUPRANGE */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-make-literal */
					{
#line 713 "src/libre/parser.act"

		(ZIrange) = ast_make_expr_literal(act_state->poolp, *flags, (ZIcrange));
		if ((ZIrange) == NULL) {
			goto ZL4;
		}
	
#line 2320 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-literal */
					/* BEGINNING OF ACTION: ast-add-alt */
					{
#line 867 "src/libre/parser.act"

		if (!ast_add_expr_alt((ZItmp), (ZIrange))) {
			goto ZL4;
		}
	
#line 2331 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-add-alt */
					/* BEGINNING OF ACTION: mark-group */
					{
#line 590 "src/libre/parser.act"

		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));
	
#line 2341 "src/libre/dialect/pcre/parser.c"
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
#line 548 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCLOSEGROUP;
		}
		goto ZL1;
	
#line 2361 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-closegroup */
				ZIend = ZIstart;
			}
		ZL3:;
		}
		/* END OF INLINE: 198 */
		/* BEGINNING OF ACTION: mark-expr */
		{
#line 605 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;

		AST_POS_OF_LX_POS(ast_start, (ZIstart));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));

/* TODO: reinstate this, applies to an expr node in general
		(ZItmp)->u.class.start = ast_start;
		(ZItmp)->u.class.end   = ast_end;
*/
	
#line 2386 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: mark-expr */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_179(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZItmp)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZIc;
			t_pos ZIrstart;
			t_pos ZI180;
			t_ast__expr ZInode1;

			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 237 "src/libre/parser.act"

		ZIc = '-';
		ZIrstart = lex_state->lx.start;
		ZI180   = lex_state->lx.end;
	
#line 2417 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			/* BEGINNING OF INLINE: 181 */
			{
				switch (CURRENT_TERMINAL) {
				default:
					{
						/* BEGINNING OF ACTION: ast-make-literal */
						{
#line 713 "src/libre/parser.act"

		(ZInode1) = ast_make_expr_literal(act_state->poolp, *flags, (ZIc));
		if ((ZInode1) == NULL) {
			goto ZL1;
		}
	
#line 2435 "src/libre/dialect/pcre/parser.c"
						}
						/* END OF ACTION: ast-make-literal */
					}
					break;
				case (TOK_RANGE):
					{
						t_endpoint ZIlower;
						t_char ZI182;
						t_pos ZI183;
						t_pos ZI184;
						t_endpoint ZIupper;
						t_pos ZIend;

						/* BEGINNING OF ACTION: ast-range-endpoint-literal */
						{
#line 678 "src/libre/parser.act"

		(ZIlower).type = AST_ENDPOINT_LITERAL;
		(ZIlower).u.literal.c = (ZIc);
	
#line 2456 "src/libre/dialect/pcre/parser.c"
						}
						/* END OF ACTION: ast-range-endpoint-literal */
						/* BEGINNING OF EXTRACT: RANGE */
						{
#line 237 "src/libre/parser.act"

		ZI182 = '-';
		ZI183 = lex_state->lx.start;
		ZI184   = lex_state->lx.end;
	
#line 2467 "src/libre/dialect/pcre/parser.c"
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
#line 821 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		unsigned char lower, upper;

		AST_POS_OF_LX_POS(ast_start, (ZIrstart));
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

		(ZInode1) = ast_make_expr_range(act_state->poolp, *flags, &(ZIlower), ast_start, &(ZIupper), ast_end);
		if ((ZInode1) == NULL) {
			goto ZL1;
		}
	
#line 2511 "src/libre/dialect/pcre/parser.c"
						}
						/* END OF ACTION: ast-make-range */
					}
					break;
				}
			}
			/* END OF INLINE: 181 */
			/* BEGINNING OF ACTION: ast-add-alt */
			{
#line 867 "src/libre/parser.act"

		if (!ast_add_expr_alt((*ZItmp), (ZInode1))) {
			goto ZL1;
		}
	
#line 2527 "src/libre/dialect/pcre/parser.c"
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
		/* BEGINNING OF INLINE: 146 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_RANGE):
				{
					t_char ZIc;
					t_pos ZI148;

					/* BEGINNING OF EXTRACT: RANGE */
					{
#line 237 "src/libre/parser.act"

		ZIc = '-';
		ZI148 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 2569 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: RANGE */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-range-endpoint-literal */
					{
#line 678 "src/libre/parser.act"

		(ZIr).type = AST_ENDPOINT_LITERAL;
		(ZIr).u.literal.c = (ZIc);
	
#line 2580 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-range-endpoint-literal */
				}
				break;
			case (TOK_NAMED__CLASS): case (TOK_ESC): case (TOK_CONTROL): case (TOK_OCT):
			case (TOK_HEX): case (TOK_CHAR):
				{
					t_pos ZI147;

					p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint (flags, lex_state, act_state, err, &ZIr, &ZI147, &ZIend);
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
		/* END OF INLINE: 146 */
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

		p_expr_C_Cpiece_C_Catom (flags, lex_state, act_state, err, &ZIe);
		/* BEGINNING OF INLINE: 243 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_OPT): case (TOK_PLUS): case (TOK_STAR): case (TOK_OPENCOUNT):
				{
					p_expr_C_Cpiece_C_Clist_Hof_Hcounts (flags, lex_state, act_state, err, ZIe, &ZInode);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				{
					t_ast__count ZIc;

					/* BEGINNING OF ACTION: count-one */
					{
#line 654 "src/libre/parser.act"

		(ZIc) = ast_make_count(1, NULL, 1, NULL);
	
#line 2646 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: count-one */
					/* BEGINNING OF ACTION: ast-make-piece */
					{
#line 724 "src/libre/parser.act"

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
	
#line 2665 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-piece */
				}
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 243 */
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

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF ACTION: ast-make-alt */
		{
#line 706 "src/libre/parser.act"

		(ZInode) = ast_make_expr_alt(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2703 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-make-alt */
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
#line 534 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXALTS;
		}
		goto ZL2;
	
#line 2724 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: err-expected-alts */
		/* BEGINNING OF ACTION: ast-make-empty */
		{
#line 692 "src/libre/parser.act"

		(ZInode) = ast_make_expr_empty(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL2;
		}
	
#line 2736 "src/libre/dialect/pcre/parser.c"
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
p_194(flags flags, lex_state lex_state, act_state act_state, err err, t_pos *ZIstart, t_char *ZIcbrak, t_ast__expr *ZOnode1)
{
	t_ast__expr ZInode1;

	switch (CURRENT_TERMINAL) {
	default:
		{
			/* BEGINNING OF ACTION: ast-make-literal */
			{
#line 713 "src/libre/parser.act"

		(ZInode1) = ast_make_expr_literal(act_state->poolp, *flags, (*ZIcbrak));
		if ((ZInode1) == NULL) {
			goto ZL1;
		}
	
#line 2765 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-literal */
		}
		break;
	case (TOK_RANGE):
		{
			t_endpoint ZIr;
			t_char ZI195;
			t_pos ZI196;
			t_pos ZI197;
			t_endpoint ZIupper;
			t_pos ZIend;
			t_endpoint ZIlower;

			/* BEGINNING OF ACTION: ast-range-endpoint-literal */
			{
#line 678 "src/libre/parser.act"

		(ZIr).type = AST_ENDPOINT_LITERAL;
		(ZIr).u.literal.c = (*ZIcbrak);
	
#line 2787 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 237 "src/libre/parser.act"

		ZI195 = '-';
		ZI196 = lex_state->lx.start;
		ZI197   = lex_state->lx.end;
	
#line 2798 "src/libre/dialect/pcre/parser.c"
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
#line 678 "src/libre/parser.act"

		(ZIlower).type = AST_ENDPOINT_LITERAL;
		(ZIlower).u.literal.c = (*ZIcbrak);
	
#line 2814 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
			/* BEGINNING OF ACTION: ast-make-range */
			{
#line 821 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		unsigned char lower, upper;

		AST_POS_OF_LX_POS(ast_start, (*ZIstart));
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

		(ZInode1) = ast_make_expr_range(act_state->poolp, *flags, &(ZIlower), ast_start, &(ZIupper), ast_end);
		if ((ZInode1) == NULL) {
			goto ZL1;
		}
	
#line 2852 "src/libre/dialect/pcre/parser.c"
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
		case (TOK_FLAGS):
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: re-flag-none */
		{
#line 630 "src/libre/parser.act"

		(ZIempty__pos) = RE_FLAGS_NONE;
	
#line 2895 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: re-flag-none */
		/* BEGINNING OF ACTION: re-flag-none */
		{
#line 630 "src/libre/parser.act"

		(ZIempty__neg) = RE_FLAGS_NONE;
	
#line 2904 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: re-flag-none */
		/* BEGINNING OF INLINE: 216 */
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
		/* END OF INLINE: 216 */
		/* BEGINNING OF INLINE: 218 */
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
		/* END OF INLINE: 218 */
		/* BEGINNING OF INLINE: 221 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSE):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-mask-re-flags */
					{
#line 753 "src/libre/parser.act"

		/*
		 * Note: in cases like `(?i-i)`, the negative is
		 * required to take precedence.
		 */
		*flags |= (ZIpos);
		*flags &= ~(ZIneg);
	
#line 2965 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-mask-re-flags */
					/* BEGINNING OF ACTION: ast-make-empty */
					{
#line 692 "src/libre/parser.act"

		(ZInode) = ast_make_expr_empty(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL5;
		}
	
#line 2977 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-empty */
				}
				break;
			case (TOK_SUB):
				{
					t_re__flags ZIflags;
					t_ast__expr ZIe;

					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-get-re-flags */
					{
#line 745 "src/libre/parser.act"

		(ZIflags) = *flags;
	
#line 2994 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-get-re-flags */
					/* BEGINNING OF ACTION: ast-mask-re-flags */
					{
#line 753 "src/libre/parser.act"

		/*
		 * Note: in cases like `(?i-i)`, the negative is
		 * required to take precedence.
		 */
		*flags |= (ZIpos);
		*flags &= ~(ZIneg);
	
#line 3008 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-mask-re-flags */
					p_expr (flags, lex_state, act_state, err, &ZIe);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL5;
					}
					/* BEGINNING OF ACTION: ast-set-re-flags */
					{
#line 749 "src/libre/parser.act"

		*flags = (ZIflags);
	
#line 3022 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-set-re-flags */
					switch (CURRENT_TERMINAL) {
					case (TOK_CLOSE):
						break;
					default:
						goto ZL5;
					}
					ADVANCE_LEXER;
					ZInode = ZIe;
				}
				break;
			default:
				goto ZL5;
			}
			goto ZL4;
		ZL5:;
			{
				/* BEGINNING OF ACTION: err-expected-closeflags */
				{
#line 569 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCLOSEFLAGS;
		}
		goto ZL1;
	
#line 3050 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-closeflags */
				/* BEGINNING OF ACTION: ast-make-empty */
				{
#line 692 "src/libre/parser.act"

		(ZInode) = ast_make_expr_empty(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 3062 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: ast-make-empty */
			}
		ZL4:;
		}
		/* END OF INLINE: 221 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Cpiece_C_Clist_Hof_Hcounts(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr ZIe, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__count ZIc;

		p_expr_C_Cpiece_C_Ccount (flags, lex_state, act_state, err, &ZIc);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
		/* BEGINNING OF ACTION: ast-make-piece */
		{
#line 724 "src/libre/parser.act"

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
	
#line 3110 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-make-piece */
		/* BEGINNING OF INLINE: 242 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_OPT):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: err-unsupported */
					{
#line 583 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXUNSUPPORTD;
		}
		goto ZL1;
	
#line 3128 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: err-unsupported */
				}
				break;
			case (TOK_PLUS):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: err-unsupported */
					{
#line 583 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXUNSUPPORTD;
		}
		goto ZL1;
	
#line 3145 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: err-unsupported */
				}
				break;
			default:
				break;
			}
		}
		/* END OF INLINE: 242 */
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
		/* BEGINNING OF INLINE: 143 */
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
			case (TOK_ESC): case (TOK_CONTROL): case (TOK_OCT): case (TOK_HEX):
			case (TOK_CHAR):
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
		/* END OF INLINE: 143 */
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
p_class_Hnamed(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode, t_pos *ZOstart, t_pos *ZOend)
{
	t_ast__expr ZInode;
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
#line 481 "src/libre/parser.act"

		ZIid = DIALECT_CLASS(lex_state->buf.a);
		if (ZIid == NULL) {
			/* syntax error -- unrecognized class */
			goto ZL1;
		}

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 3241 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: NAMED_CLASS */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: ast-make-named */
		{
#line 854 "src/libre/parser.act"

		(ZInode) = ast_make_expr_named(act_state->poolp, *flags, (ZIid));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 3258 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-make-named */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
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
#line 867 "src/libre/parser.act"

		if (!ast_add_expr_alt((ZIalts), (ZIa))) {
			goto ZL1;
		}
	
#line 3295 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-add-alt */
		/* BEGINNING OF INLINE: 256 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ALT):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF INLINE: expr::list-of-alts */
					goto ZL2_expr_C_Clist_Hof_Halts;
					/* END OF INLINE: expr::list-of-alts */
				}
				/* UNREACHED */
			default:
				break;
			}
		}
		/* END OF INLINE: 256 */
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-alts */
		{
#line 534 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXALTS;
		}
		goto ZL4;
	
#line 3327 "src/libre/dialect/pcre/parser.c"
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
			t_pos ZI293;
			t_pos ZI294;
			t_unsigned ZIm;

			/* BEGINNING OF EXTRACT: OPENCOUNT */
			{
#line 275 "src/libre/parser.act"

		ZI293 = lex_state->lx.start;
		ZI294   = lex_state->lx.end;
	
#line 3357 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: OPENCOUNT */
			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (TOK_COUNT):
				/* BEGINNING OF EXTRACT: COUNT */
				{
#line 461 "src/libre/parser.act"

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
	
#line 3385 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF EXTRACT: COUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			p_295 (flags, lex_state, act_state, err, &ZI293, &ZIm, &ZIc);
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
#line 650 "src/libre/parser.act"

		(ZIc) = ast_make_count(0, NULL, 1, NULL);
	
#line 3409 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: count-zero-or-one */
		}
		break;
	case (TOK_PLUS):
		{
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: count-one-or-more */
			{
#line 646 "src/libre/parser.act"

		(ZIc) = ast_make_count(1, NULL, AST_COUNT_UNBOUNDED, NULL);
	
#line 3423 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: count-one-or-more */
		}
		break;
	case (TOK_STAR):
		{
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: count-zero-or-more */
			{
#line 642 "src/libre/parser.act"

		(ZIc) = ast_make_count(0, NULL, AST_COUNT_UNBOUNDED, NULL);
	
#line 3437 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: count-zero-or-more */
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
		/* BEGINNING OF ACTION: err-expected-count */
		{
#line 520 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCOUNT;
		}
		goto ZL2;
	
#line 3459 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: err-expected-count */
		/* BEGINNING OF ACTION: count-one */
		{
#line 654 "src/libre/parser.act"

		(ZIc) = ast_make_count(1, NULL, 1, NULL);
	
#line 3468 "src/libre/dialect/pcre/parser.c"
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

void
p_re__pcre(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 258 */
		{
			{
				p_expr (flags, lex_state, act_state, err, &ZInode);
				if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
					RESTORE_LEXER;
					goto ZL1;
				}
			}
		}
		/* END OF INLINE: 258 */
		/* BEGINNING OF INLINE: 259 */
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
#line 576 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXEOF;
		}
		goto ZL1;
	
#line 3523 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL3:;
		}
		/* END OF INLINE: 259 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
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
#line 620 "src/libre/parser.act"

		/* TODO: or the unicode equivalent */
		(ZIa) = &class_any;
	
#line 3557 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: class-any */
			/* BEGINNING OF ACTION: ast-make-named */
			{
#line 854 "src/libre/parser.act"

		(ZIe) = ast_make_expr_named(act_state->poolp, *flags, (ZIa));
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 3569 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-named */
		}
		break;
	case (TOK_END):
		{
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-make-anchor-end */
			{
#line 769 "src/libre/parser.act"

		(ZIe) = ast_make_expr_anchor(act_state->poolp, *flags, AST_ANCHOR_END);
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 3586 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-anchor-end */
		}
		break;
	case (TOK_OPEN):
		{
			t_re__flags ZIflags;
			t_ast__expr ZIg;

			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-get-re-flags */
			{
#line 745 "src/libre/parser.act"

		(ZIflags) = *flags;
	
#line 3603 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-get-re-flags */
			p_expr (flags, lex_state, act_state, err, &ZIg);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: ast-set-re-flags */
			{
#line 749 "src/libre/parser.act"

		*flags = (ZIflags);
	
#line 3617 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-set-re-flags */
			/* BEGINNING OF ACTION: ast-make-group */
			{
#line 738 "src/libre/parser.act"

		(ZIe) = ast_make_expr_group(act_state->poolp, *flags, (ZIg));
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 3629 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-group */
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSE):
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
#line 762 "src/libre/parser.act"

		(ZIe) = ast_make_expr_anchor(act_state->poolp, *flags, AST_ANCHOR_START);
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 3653 "src/libre/dialect/pcre/parser.c"
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
	case (TOK_FLAGS):
		{
			p_expr_C_Cflags (flags, lex_state, act_state, err, &ZIe);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_ESC): case (TOK_NOESC): case (TOK_CONTROL): case (TOK_OCT):
	case (TOK_HEX): case (TOK_CHAR): case (TOK_UNSUPPORTED):
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
#line 527 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXATOM;
		}
		goto ZL2;
	
#line 3712 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: err-expected-atom */
		/* BEGINNING OF ACTION: ast-make-empty */
		{
#line 692 "src/libre/parser.act"

		(ZIe) = ast_make_expr_empty(act_state->poolp, *flags);
		if ((ZIe) == NULL) {
			goto ZL2;
		}
	
#line 3724 "src/libre/dialect/pcre/parser.c"
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
p_expr_C_Calt(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_ANY): case (TOK_START): case (TOK_END): case (TOK_OPEN):
	case (TOK_OPENGROUP): case (TOK_OPENGROUPINV): case (TOK_OPENGROUPCB): case (TOK_OPENGROUPINVCB):
	case (TOK_NAMED__CLASS): case (TOK_FLAGS): case (TOK_ESC): case (TOK_NOESC):
	case (TOK_CONTROL): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
	case (TOK_UNSUPPORTED): case (TOK_INVALID__COMMENT):
		{
			/* BEGINNING OF ACTION: ast-make-concat */
			{
#line 699 "src/libre/parser.act"

		(ZInode) = ast_make_expr_concat(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 3757 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-concat */
			p_expr_C_Clist_Hof_Hpieces (flags, lex_state, act_state, err, ZInode);
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
#line 692 "src/libre/parser.act"

		(ZInode) = ast_make_expr_empty(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 3778 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-empty */
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
p_expr_C_Ctype(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__expr ZIclass;
		t_pos ZIstart;
		t_pos ZIend;

		p_class_Hnamed (flags, lex_state, act_state, err, &ZIclass, &ZIstart, &ZIend);
		if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
			RESTORE_LEXER;
			goto ZL1;
		}
		/* BEGINNING OF ACTION: ast-make-alt */
		{
#line 706 "src/libre/parser.act"

		(ZInode) = ast_make_expr_alt(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 3821 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-make-alt */
		/* BEGINNING OF ACTION: ast-add-alt */
		{
#line 867 "src/libre/parser.act"

		if (!ast_add_expr_alt((ZInode), (ZIclass))) {
			goto ZL1;
		}
	
#line 3832 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-add-alt */
		/* BEGINNING OF ACTION: mark-expr */
		{
#line 605 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;

		AST_POS_OF_LX_POS(ast_start, (ZIstart));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));

/* TODO: reinstate this, applies to an expr node in general
		(ZInode)->u.class.start = ast_start;
		(ZInode)->u.class.end   = ast_end;
*/
	
#line 3852 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: mark-expr */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

/* BEGINNING OF TRAILER */

#line 873 "src/libre/parser.act"


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
		struct ast_expr_pool **poolp,
		enum re_flags flags, int overlap,
		struct re_err *err)
	{
		struct ast *ast;

		struct act_state  act_state_s;
		struct act_state *act_state;
		struct lex_state  lex_state_s;
		struct lex_state *lex_state;
		struct re_err dummy;

		struct LX_STATE *lx;

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
		act_state->poolp   = poolp;

		err->e = RE_ESUCCESS;

		ADVANCE_LEXER;
		DIALECT_ENTRY(&flags, lex_state, act_state, err, &ast->expr);

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

#line 4008 "src/libre/dialect/pcre/parser.c"

/* END OF FILE */
