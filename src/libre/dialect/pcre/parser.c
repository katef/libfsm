/*
 * Automatically generated from the files:
 *	src/libre/dialect/pcre/parser.sid
 * and
 *	src/libre/parser.act
 * by:
 *	sid
 */

/* BEGINNING OF HEADER */

#line 142 "src/libre/parser.act"


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

	struct flags {
		enum re_flags flags;
		struct flags *parent;
	};

	typedef char     t_char;
	typedef unsigned t_unsigned;
	typedef unsigned t_pred; /* TODO */

	typedef struct lx_pos t_pos;
	typedef enum re_flags t_re__flags;
	typedef const struct class * t_ast__class__id;
	typedef struct ast_count t_ast__count;
	typedef struct ast_endpoint t_endpoint;

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

#line 213 "src/libre/dialect/pcre/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_expr_C_Ccharacter_Hclass_C_Cclass_Hhead(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Cflags_C_Cflag__set(flags, lex_state, act_state, err, t_re__flags, t_re__flags *);
static void p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_C_Crange_Hendpoint_Hliteral(flags, lex_state, act_state, err, t_endpoint *, t_pos *, t_pos *);
static void p_265(flags, lex_state, act_state, err, t_ast__class__id *, t_pos *, t_ast__expr *);
static void p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms(flags, lex_state, act_state, err, t_ast__expr);
static void p_145(flags, lex_state, act_state, err);
static void p_285(flags, lex_state, act_state, err, t_char *, t_pos *, t_ast__expr *);
static void p_288(flags, lex_state, act_state, err, t_pos *, t_unsigned *, t_ast__count *);
static void p_289(flags, lex_state, act_state, err, t_pos *, t_unsigned *, t_ast__count *);
static void p_expr_C_Clist_Hof_Hpieces(flags, lex_state, act_state, err, t_ast__expr);
static void p_expr_C_Cliteral(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Ccharacter_Hclass_C_Cclass_Hterm(flags, lex_state, act_state, err, t_ast__expr *);
static void p_173(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_C_Crange_Hendpoint_Hclass(flags, lex_state, act_state, err, t_endpoint *, t_pos *, t_pos *);
static void p_expr_C_Ccharacter_Hclass(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_Hend(flags, lex_state, act_state, err, t_endpoint *, t_pos *);
static void p_190(flags, lex_state, act_state, err, t_pos *, t_char *, t_ast__expr *);
static void p_expr_C_Cpiece(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr(flags, lex_state, act_state, err, t_ast__expr *);
static void p_194(flags, lex_state, act_state, err, t_ast__expr *);
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
p_expr_C_Ccharacter_Hclass_C_Cclass_Hhead(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZIclass)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_INVERT):
		{
			t_char ZI109;

			/* BEGINNING OF EXTRACT: INVERT */
			{
#line 237 "src/libre/parser.act"

		ZI109 = '^';
	
#line 272 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: INVERT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-make-invert */
			{
#line 773 "src/libre/parser.act"

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

		any = ast_make_expr_named(&class_any);
		if (any == NULL) {
			goto ZL1;
		}

		(*ZIclass) = ast_make_expr_subtract(any, (*ZIclass));
		if ((*ZIclass) == NULL) {
			goto ZL1;
		}
	
#line 316 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-invert */
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
p_expr_C_Cflags_C_Cflag__set(flags flags, lex_state lex_state, act_state act_state, err err, t_re__flags ZIi, t_re__flags *ZOo)
{
	t_re__flags ZIo;

	switch (CURRENT_TERMINAL) {
	case (TOK_FLAG__INSENSITIVE):
		{
			t_re__flags ZIc;

			/* BEGINNING OF EXTRACT: FLAG_INSENSITIVE */
			{
#line 452 "src/libre/parser.act"

		ZIc = RE_ICASE;
	
#line 348 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: FLAG_INSENSITIVE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: re-flag-union */
			{
#line 582 "src/libre/parser.act"

		(ZIo) = (ZIi) | (ZIc);
	
#line 358 "src/libre/dialect/pcre/parser.c"
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
#line 518 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EFLAG;
		}
		goto ZL1;
	
#line 376 "src/libre/dialect/pcre/parser.c"
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

		/* BEGINNING OF INLINE: 132 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 414 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		ZIc = lex_state->buf.a[0];
	
#line 424 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_CONTROL):
				{
					/* BEGINNING OF EXTRACT: CONTROL */
					{
#line 320 "src/libre/parser.act"

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
	
#line 450 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CONTROL */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: err-unsupported */
					{
#line 539 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXUNSUPPORTD;
		}
		goto ZL1;
	
#line 463 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: err-unsupported */
				}
				break;
			case (TOK_ESC):
				{
					/* BEGINNING OF EXTRACT: ESC */
					{
#line 287 "src/libre/parser.act"

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
	
#line 493 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: ESC */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_HEX):
				{
					/* BEGINNING OF EXTRACT: HEX */
					{
#line 378 "src/libre/parser.act"

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
	
#line 543 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: HEX */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_OCT):
				{
					/* BEGINNING OF EXTRACT: OCT */
					{
#line 338 "src/libre/parser.act"

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
	
#line 593 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OCT */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 132 */
		/* BEGINNING OF ACTION: ast-range-endpoint-literal */
		{
#line 626 "src/libre/parser.act"

		(ZIr).type = AST_ENDPOINT_LITERAL;
		(ZIr).u.literal.c = (ZIc);
	
#line 611 "src/libre/dialect/pcre/parser.c"
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
p_265(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__class__id *ZI262, t_pos *ZI263, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	default:
		{
			/* BEGINNING OF ACTION: ast-make-named */
			{
#line 816 "src/libre/parser.act"

		(ZInode) = ast_make_expr_named((*ZI262));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 642 "src/libre/dialect/pcre/parser.c"
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
#line 631 "src/libre/parser.act"

		(ZIlower).type = AST_ENDPOINT_NAMED;
		(ZIlower).u.named.class = (*ZI262);
	
#line 660 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-class */
			p_145 (flags, lex_state, act_state, err);
			p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_Hend (flags, lex_state, act_state, err, &ZIupper, &ZIend);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: mark-range */
			{
#line 548 "src/libre/parser.act"

		mark(&act_state->rangestart, &(*ZI263));
		mark(&act_state->rangeend,   &(ZIend));
	
#line 676 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-range */
			/* BEGINNING OF ACTION: ast-make-range */
			{
#line 786 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		unsigned char lower, upper;

		AST_POS_OF_LX_POS(ast_start, (*ZI263));
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

		(ZInode) = ast_make_expr_range(&(ZIlower), ast_start, &(ZIupper), ast_end);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 714 "src/libre/dialect/pcre/parser.c"
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
p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr ZIclass)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms:;
	{
		/* BEGINNING OF INLINE: 156 */
		{
			{
				t_ast__expr ZInode;

				p_expr_C_Ccharacter_Hclass_C_Cclass_Hterm (flags, lex_state, act_state, err, &ZInode);
				if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
					RESTORE_LEXER;
					goto ZL4;
				}
				/* BEGINNING OF ACTION: ast-add-alt */
				{
#line 829 "src/libre/parser.act"

		if (!ast_add_expr_alt((ZIclass), (ZInode))) {
			goto ZL4;
		}
	
#line 756 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: ast-add-alt */
			}
			goto ZL3;
		ZL4:;
			{
				/* BEGINNING OF ACTION: err-expected-term */
				{
#line 469 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXTERM;
		}
		goto ZL1;
	
#line 772 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-term */
			}
		ZL3:;
		}
		/* END OF INLINE: 156 */
		p_194 (flags, lex_state, act_state, err, &ZIclass);
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
p_145(flags flags, lex_state lex_state, act_state act_state, err err)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZI146;
		t_pos ZI147;
		t_pos ZI148;

		switch (CURRENT_TERMINAL) {
		case (TOK_RANGE):
			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 241 "src/libre/parser.act"

		ZI146 = '-';
		ZI147 = lex_state->lx.start;
		ZI148   = lex_state->lx.end;
	
#line 812 "src/libre/dialect/pcre/parser.c"
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
#line 497 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXRANGE;
		}
		goto ZL2;
	
#line 833 "src/libre/dialect/pcre/parser.c"
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
p_285(flags flags, lex_state lex_state, act_state act_state, err err, t_char *ZI282, t_pos *ZI283, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	default:
		{
			/* BEGINNING OF ACTION: ast-make-literal */
			{
#line 678 "src/libre/parser.act"

		(ZInode) = ast_make_expr_literal((*ZI282));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 861 "src/libre/dialect/pcre/parser.c"
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
#line 626 "src/libre/parser.act"

		(ZIlower).type = AST_ENDPOINT_LITERAL;
		(ZIlower).u.literal.c = (*ZI282);
	
#line 879 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
			p_145 (flags, lex_state, act_state, err);
			p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_Hend (flags, lex_state, act_state, err, &ZIupper, &ZIend);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: mark-range */
			{
#line 548 "src/libre/parser.act"

		mark(&act_state->rangestart, &(*ZI283));
		mark(&act_state->rangeend,   &(ZIend));
	
#line 895 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-range */
			/* BEGINNING OF ACTION: ast-make-range */
			{
#line 786 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		unsigned char lower, upper;

		AST_POS_OF_LX_POS(ast_start, (*ZI283));
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

		(ZInode) = ast_make_expr_range(&(ZIlower), ast_start, &(ZIupper), ast_end);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 933 "src/libre/dialect/pcre/parser.c"
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
p_288(flags flags, lex_state lex_state, act_state act_state, err err, t_pos *ZI286, t_unsigned *ZIm, t_ast__count *ZOc)
{
	t_ast__count ZIc;

	switch (CURRENT_TERMINAL) {
	case (TOK_CLOSECOUNT):
		{
			t_pos ZI229;
			t_pos ZIend;

			/* BEGINNING OF EXTRACT: CLOSECOUNT */
			{
#line 278 "src/libre/parser.act"

		ZI229 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 967 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: CLOSECOUNT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: mark-count */
			{
#line 553 "src/libre/parser.act"

		mark(&act_state->countstart, &(*ZI286));
		mark(&act_state->countend,   &(ZIend));
	
#line 978 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-count */
			/* BEGINNING OF ACTION: count-range */
			{
#line 608 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;

		if ((*ZIm) < (*ZIm)) {
			err->e = RE_ENEGCOUNT;
			err->m = (*ZIm);
			err->n = (*ZIm);

			mark(&act_state->countstart, &(*ZI286));
			mark(&act_state->countend,   &(ZIend));

			goto ZL1;
		}

		AST_POS_OF_LX_POS(ast_start, (*ZI286));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		(ZIc) = ast_make_count((*ZIm), &ast_start, (*ZIm), &ast_end);
	
#line 1003 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: count-range */
		}
		break;
	case (TOK_SEP):
		{
			ADVANCE_LEXER;
			p_289 (flags, lex_state, act_state, err, ZI286, ZIm, &ZIc);
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
p_289(flags flags, lex_state lex_state, act_state act_state, err err, t_pos *ZI286, t_unsigned *ZIm, t_ast__count *ZOc)
{
	t_ast__count ZIc;

	switch (CURRENT_TERMINAL) {
	case (TOK_CLOSECOUNT):
		{
			t_pos ZI234;
			t_pos ZIend;
			t_unsigned ZIn;

			/* BEGINNING OF EXTRACT: CLOSECOUNT */
			{
#line 278 "src/libre/parser.act"

		ZI234 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 1050 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: CLOSECOUNT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: mark-count */
			{
#line 553 "src/libre/parser.act"

		mark(&act_state->countstart, &(*ZI286));
		mark(&act_state->countend,   &(ZIend));
	
#line 1061 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-count */
			/* BEGINNING OF ACTION: count-unbounded */
			{
#line 586 "src/libre/parser.act"

		(ZIn) = AST_COUNT_UNBOUNDED;
	
#line 1070 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: count-unbounded */
			/* BEGINNING OF ACTION: count-range */
			{
#line 608 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;

		if ((ZIn) < (*ZIm)) {
			err->e = RE_ENEGCOUNT;
			err->m = (*ZIm);
			err->n = (ZIn);

			mark(&act_state->countstart, &(*ZI286));
			mark(&act_state->countend,   &(ZIend));

			goto ZL1;
		}

		AST_POS_OF_LX_POS(ast_start, (*ZI286));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		(ZIc) = ast_make_count((*ZIm), &ast_start, (ZIn), &ast_end);
	
#line 1095 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: count-range */
		}
		break;
	case (TOK_COUNT):
		{
			t_unsigned ZIn;
			t_pos ZI232;
			t_pos ZIend;

			/* BEGINNING OF EXTRACT: COUNT */
			{
#line 429 "src/libre/parser.act"

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
	
#line 1128 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: COUNT */
			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSECOUNT):
				/* BEGINNING OF EXTRACT: CLOSECOUNT */
				{
#line 278 "src/libre/parser.act"

		ZI232 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 1141 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF EXTRACT: CLOSECOUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: mark-count */
			{
#line 553 "src/libre/parser.act"

		mark(&act_state->countstart, &(*ZI286));
		mark(&act_state->countend,   &(ZIend));
	
#line 1156 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-count */
			/* BEGINNING OF ACTION: count-range */
			{
#line 608 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;

		if ((ZIn) < (*ZIm)) {
			err->e = RE_ENEGCOUNT;
			err->m = (*ZIm);
			err->n = (ZIn);

			mark(&act_state->countstart, &(*ZI286));
			mark(&act_state->countend,   &(ZIend));

			goto ZL1;
		}

		AST_POS_OF_LX_POS(ast_start, (*ZI286));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		(ZIc) = ast_make_count((*ZIm), &ast_start, (ZIn), &ast_end);
	
#line 1181 "src/libre/dialect/pcre/parser.c"
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
#line 823 "src/libre/parser.act"

		if (!ast_add_expr_concat((ZIcat), (ZIa))) {
			goto ZL1;
		}
	
#line 1222 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-add-concat */
		/* BEGINNING OF INLINE: 243 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY): case (TOK_START): case (TOK_END): case (TOK_OPENSUB):
			case (TOK_OPENCAPTURE): case (TOK_OPENGROUP): case (TOK_OPENGROUPINV): case (TOK_OPENGROUPCB):
			case (TOK_OPENGROUPINVCB): case (TOK_NAMED__CLASS): case (TOK_OPENFLAGS): case (TOK_ESC):
			case (TOK_CONTROL): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
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
		/* END OF INLINE: 243 */
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

		/* BEGINNING OF INLINE: 94 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					t_pos ZI102;
					t_pos ZI103;

					/* BEGINNING OF EXTRACT: CHAR */
					{
#line 414 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI102 = lex_state->lx.start;
		ZI103   = lex_state->lx.end;

		ZIc = lex_state->buf.a[0];
	
#line 1281 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_ESC):
				{
					t_pos ZI96;
					t_pos ZI97;

					/* BEGINNING OF EXTRACT: ESC */
					{
#line 287 "src/libre/parser.act"

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

		ZI96 = lex_state->lx.start;
		ZI97   = lex_state->lx.end;
	
#line 1315 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: ESC */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_HEX):
				{
					t_pos ZI100;
					t_pos ZI101;

					/* BEGINNING OF EXTRACT: HEX */
					{
#line 378 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 3);

		ZI100 = lex_state->lx.start;
		ZI101   = lex_state->lx.end;

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
	
#line 1368 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: HEX */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_OCT):
				{
					t_pos ZI98;
					t_pos ZI99;

					/* BEGINNING OF EXTRACT: OCT */
					{
#line 338 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		ZI98 = lex_state->lx.start;
		ZI99   = lex_state->lx.end;

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
	
#line 1421 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OCT */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 94 */
		/* BEGINNING OF ACTION: ast-make-literal */
		{
#line 678 "src/libre/parser.act"

		(ZInode) = ast_make_expr_literal((ZIc));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1441 "src/libre/dialect/pcre/parser.c"
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
			t_char ZI278;
			t_pos ZI279;
			t_pos ZI280;

			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 414 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI279 = lex_state->lx.start;
		ZI280   = lex_state->lx.end;

		ZI278 = lex_state->buf.a[0];
	
#line 1477 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			ADVANCE_LEXER;
			p_285 (flags, lex_state, act_state, err, &ZI278, &ZI279, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_CONTROL):
		{
			t_char ZI282;
			t_pos ZI283;
			t_pos ZI284;

			/* BEGINNING OF EXTRACT: CONTROL */
			{
#line 320 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] == 'c');
		assert(lex_state->buf.a[2] != '\0');
		assert(lex_state->buf.a[3] == '\0');

		ZI282 = lex_state->buf.a[2];
		if ((unsigned char) ZI282 > 127) {
			goto ZL1;
		}
		ZI282 = (((toupper((unsigned char)ZI282)) - 64) % 128 + 128) % 128;

		ZI283 = lex_state->lx.start;
		ZI284   = lex_state->lx.end;
	
#line 1512 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: CONTROL */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: err-unsupported */
			{
#line 539 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXUNSUPPORTD;
		}
		goto ZL1;
	
#line 1525 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: err-unsupported */
			p_285 (flags, lex_state, act_state, err, &ZI282, &ZI283, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_ESC):
		{
			t_char ZI266;
			t_pos ZI267;
			t_pos ZI268;

			/* BEGINNING OF EXTRACT: ESC */
			{
#line 287 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZI266 = lex_state->buf.a[1];

		switch (ZI266) {
		case 'a': ZI266 = '\a'; break;
		case 'f': ZI266 = '\f'; break;
		case 'n': ZI266 = '\n'; break;
		case 'r': ZI266 = '\r'; break;
		case 't': ZI266 = '\t'; break;
		case 'v': ZI266 = '\v'; break;
		default:             break;
		}

		ZI267 = lex_state->lx.start;
		ZI268   = lex_state->lx.end;
	
#line 1564 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: ESC */
			ADVANCE_LEXER;
			p_285 (flags, lex_state, act_state, err, &ZI266, &ZI267, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_HEX):
		{
			t_char ZI274;
			t_pos ZI275;
			t_pos ZI276;

			/* BEGINNING OF EXTRACT: HEX */
			{
#line 378 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 3);

		ZI275 = lex_state->lx.start;
		ZI276   = lex_state->lx.end;

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

		ZI274 = (char) (unsigned char) u;
	
#line 1623 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: HEX */
			ADVANCE_LEXER;
			p_285 (flags, lex_state, act_state, err, &ZI274, &ZI275, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_NAMED__CLASS):
		{
			t_ast__class__id ZI262;
			t_pos ZI263;
			t_pos ZI264;

			/* BEGINNING OF EXTRACT: NAMED_CLASS */
			{
#line 441 "src/libre/parser.act"

		ZI262 = DIALECT_CLASS(lex_state->buf.a);
		if (ZI262 == NULL) {
			/* syntax error -- unrecognized class */
			goto ZL1;
		}

		ZI263 = lex_state->lx.start;
		ZI264   = lex_state->lx.end;
	
#line 1653 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: NAMED_CLASS */
			ADVANCE_LEXER;
			p_265 (flags, lex_state, act_state, err, &ZI262, &ZI263, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_OCT):
		{
			t_char ZI270;
			t_pos ZI271;
			t_pos ZI272;

			/* BEGINNING OF EXTRACT: OCT */
			{
#line 338 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		ZI271 = lex_state->lx.start;
		ZI272   = lex_state->lx.end;

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

		ZI270 = (char) (unsigned char) u;
	
#line 1712 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: OCT */
			ADVANCE_LEXER;
			p_285 (flags, lex_state, act_state, err, &ZI270, &ZI271, &ZInode);
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
p_173(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZItmp)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZIc;
			t_pos ZIrstart;
			t_pos ZI174;
			t_ast__expr ZInode1;

			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 241 "src/libre/parser.act"

		ZIc = '-';
		ZIrstart = lex_state->lx.start;
		ZI174   = lex_state->lx.end;
	
#line 1755 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			/* BEGINNING OF INLINE: 175 */
			{
				switch (CURRENT_TERMINAL) {
				default:
					{
						/* BEGINNING OF ACTION: ast-make-literal */
						{
#line 678 "src/libre/parser.act"

		(ZInode1) = ast_make_expr_literal((ZIc));
		if ((ZInode1) == NULL) {
			goto ZL1;
		}
	
#line 1773 "src/libre/dialect/pcre/parser.c"
						}
						/* END OF ACTION: ast-make-literal */
					}
					break;
				case (TOK_RANGE):
					{
						t_endpoint ZIlower;
						t_char ZI176;
						t_pos ZI177;
						t_pos ZI178;
						t_endpoint ZIupper;
						t_pos ZIend;

						/* BEGINNING OF ACTION: ast-range-endpoint-literal */
						{
#line 626 "src/libre/parser.act"

		(ZIlower).type = AST_ENDPOINT_LITERAL;
		(ZIlower).u.literal.c = (ZIc);
	
#line 1794 "src/libre/dialect/pcre/parser.c"
						}
						/* END OF ACTION: ast-range-endpoint-literal */
						/* BEGINNING OF EXTRACT: RANGE */
						{
#line 241 "src/libre/parser.act"

		ZI176 = '-';
		ZI177 = lex_state->lx.start;
		ZI178   = lex_state->lx.end;
	
#line 1805 "src/libre/dialect/pcre/parser.c"
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
#line 786 "src/libre/parser.act"

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

		(ZInode1) = ast_make_expr_range(&(ZIlower), ast_start, &(ZIupper), ast_end);
		if ((ZInode1) == NULL) {
			goto ZL1;
		}
	
#line 1849 "src/libre/dialect/pcre/parser.c"
						}
						/* END OF ACTION: ast-make-range */
					}
					break;
				}
			}
			/* END OF INLINE: 175 */
			/* BEGINNING OF ACTION: ast-add-alt */
			{
#line 829 "src/libre/parser.act"

		if (!ast_add_expr_alt((*ZItmp), (ZInode1))) {
			goto ZL1;
		}
	
#line 1865 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-add-alt */
			p_194 (flags, lex_state, act_state, err, ZItmp);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_NAMED__CLASS): case (TOK_ESC): case (TOK_CONTROL): case (TOK_OCT):
	case (TOK_HEX): case (TOK_CHAR):
		{
			p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms (flags, lex_state, act_state, err, *ZItmp);
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
#line 441 "src/libre/parser.act"

		ZIid = DIALECT_CLASS(lex_state->buf.a);
		if (ZIid == NULL) {
			/* syntax error -- unrecognized class */
			goto ZL1;
		}

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 1924 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: NAMED_CLASS */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: ast-range-endpoint-class */
		{
#line 631 "src/libre/parser.act"

		(ZIr).type = AST_ENDPOINT_NAMED;
		(ZIr).u.named.class = (ZIid);
	
#line 1939 "src/libre/dialect/pcre/parser.c"
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

		/* BEGINNING OF INLINE: 158 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_OPENGROUP):
				{
					t_pos ZI159;

					/* BEGINNING OF EXTRACT: OPENGROUP */
					{
#line 247 "src/libre/parser.act"

		ZIstart = lex_state->lx.start;
		ZI159   = lex_state->lx.end;
	
#line 1980 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OPENGROUP */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-make-alt */
					{
#line 671 "src/libre/parser.act"

		(ZInode) = ast_make_expr_alt();
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1993 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-alt */
					ZItmp = ZInode;
					p_expr_C_Ccharacter_Hclass_C_Cclass_Hhead (flags, lex_state, act_state, err, &ZInode);
					p_173 (flags, lex_state, act_state, err, &ZItmp);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_OPENGROUPCB):
				{
					t_pos ZI180;
					t_char ZIcbrak;
					t_ast__expr ZInode1;

					/* BEGINNING OF EXTRACT: OPENGROUPCB */
					{
#line 257 "src/libre/parser.act"

		ZIstart = lex_state->lx.start;
		ZI180   = lex_state->lx.end;
	
#line 2018 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OPENGROUPCB */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-make-alt */
					{
#line 671 "src/libre/parser.act"

		(ZInode) = ast_make_expr_alt();
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2031 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-alt */
					ZItmp = ZInode;
					/* BEGINNING OF ACTION: make-literal-cbrak */
					{
#line 685 "src/libre/parser.act"

		(ZIcbrak) = ']';
	
#line 2041 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: make-literal-cbrak */
					p_190 (flags, lex_state, act_state, err, &ZIstart, &ZIcbrak, &ZInode1);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
					/* BEGINNING OF ACTION: ast-add-alt */
					{
#line 829 "src/libre/parser.act"

		if (!ast_add_expr_alt((ZItmp), (ZInode1))) {
			goto ZL1;
		}
	
#line 2057 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-add-alt */
					p_194 (flags, lex_state, act_state, err, &ZItmp);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_OPENGROUPINV):
				{
					t_pos ZI171;

					/* BEGINNING OF EXTRACT: OPENGROUPINV */
					{
#line 252 "src/libre/parser.act"

		ZIstart = lex_state->lx.start;
		ZI171   = lex_state->lx.end;
	
#line 2078 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OPENGROUPINV */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-make-alt */
					{
#line 671 "src/libre/parser.act"

		(ZInode) = ast_make_expr_alt();
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2091 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-alt */
					ZItmp = ZInode;
					/* BEGINNING OF ACTION: ast-make-invert */
					{
#line 773 "src/libre/parser.act"

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

		any = ast_make_expr_named(&class_any);
		if (any == NULL) {
			goto ZL1;
		}

		(ZInode) = ast_make_expr_subtract(any, (ZInode));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2135 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-invert */
					p_173 (flags, lex_state, act_state, err, &ZItmp);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_OPENGROUPINVCB):
				{
					t_pos ZI188;
					t_char ZIcbrak;
					t_ast__expr ZInode1;

					/* BEGINNING OF EXTRACT: OPENGROUPINVCB */
					{
#line 262 "src/libre/parser.act"

		ZIstart = lex_state->lx.start;
		ZI188   = lex_state->lx.end;
	
#line 2158 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OPENGROUPINVCB */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-make-alt */
					{
#line 671 "src/libre/parser.act"

		(ZInode) = ast_make_expr_alt();
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2171 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-alt */
					ZItmp = ZInode;
					/* BEGINNING OF ACTION: ast-make-invert */
					{
#line 773 "src/libre/parser.act"

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

		any = ast_make_expr_named(&class_any);
		if (any == NULL) {
			goto ZL1;
		}

		(ZInode) = ast_make_expr_subtract(any, (ZInode));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2215 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-invert */
					/* BEGINNING OF ACTION: make-literal-cbrak */
					{
#line 685 "src/libre/parser.act"

		(ZIcbrak) = ']';
	
#line 2224 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: make-literal-cbrak */
					p_190 (flags, lex_state, act_state, err, &ZIstart, &ZIcbrak, &ZInode1);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
					/* BEGINNING OF ACTION: ast-add-alt */
					{
#line 829 "src/libre/parser.act"

		if (!ast_add_expr_alt((ZItmp), (ZInode1))) {
			goto ZL1;
		}
	
#line 2240 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-add-alt */
					p_194 (flags, lex_state, act_state, err, &ZItmp);
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
		/* END OF INLINE: 158 */
		/* BEGINNING OF INLINE: 195 */
		{
			{
				t_char ZI196;
				t_pos ZI197;

				switch (CURRENT_TERMINAL) {
				case (TOK_CLOSEGROUP):
					/* BEGINNING OF EXTRACT: CLOSEGROUP */
					{
#line 267 "src/libre/parser.act"

		ZI196 = ']';
		ZI197 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 2271 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CLOSEGROUP */
					break;
				default:
					goto ZL4;
				}
				ADVANCE_LEXER;
				/* BEGINNING OF ACTION: mark-group */
				{
#line 543 "src/libre/parser.act"

		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));
	
#line 2286 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: mark-group */
			}
			goto ZL3;
		ZL4:;
			{
				/* BEGINNING OF ACTION: err-expected-closegroup */
				{
#line 504 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCLOSEGROUP;
		}
		goto ZL1;
	
#line 2302 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-closegroup */
				ZIend = ZIstart;
			}
		ZL3:;
		}
		/* END OF INLINE: 195 */
		/* BEGINNING OF ACTION: mark-expr */
		{
#line 560 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;

		AST_POS_OF_LX_POS(ast_start, (ZIstart));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));

/* TODO: reinstate this, applies to an expr node in general
		(ZItmp)->u.class.start = ast_start;
		(ZItmp)->u.class.end   = ast_end;
*/
	
#line 2327 "src/libre/dialect/pcre/parser.c"
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
p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint_Hend(flags flags, lex_state lex_state, act_state act_state, err err, t_endpoint *ZOr, t_pos *ZOend)
{
	t_endpoint ZIr;
	t_pos ZIend;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 138 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_RANGE):
				{
					t_char ZIc;
					t_pos ZI140;

					/* BEGINNING OF EXTRACT: RANGE */
					{
#line 241 "src/libre/parser.act"

		ZIc = '-';
		ZI140 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 2365 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: RANGE */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-range-endpoint-literal */
					{
#line 626 "src/libre/parser.act"

		(ZIr).type = AST_ENDPOINT_LITERAL;
		(ZIr).u.literal.c = (ZIc);
	
#line 2376 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-range-endpoint-literal */
				}
				break;
			case (TOK_NAMED__CLASS): case (TOK_ESC): case (TOK_CONTROL): case (TOK_OCT):
			case (TOK_HEX): case (TOK_CHAR):
				{
					t_pos ZI139;

					p_expr_C_Ccharacter_Hclass_C_Crange_Hendpoint (flags, lex_state, act_state, err, &ZIr, &ZI139, &ZIend);
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
		/* END OF INLINE: 138 */
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
p_190(flags flags, lex_state lex_state, act_state act_state, err err, t_pos *ZIstart, t_char *ZIcbrak, t_ast__expr *ZOnode1)
{
	t_ast__expr ZInode1;

	switch (CURRENT_TERMINAL) {
	default:
		{
			/* BEGINNING OF ACTION: ast-make-literal */
			{
#line 678 "src/libre/parser.act"

		(ZInode1) = ast_make_expr_literal((*ZIcbrak));
		if ((ZInode1) == NULL) {
			goto ZL1;
		}
	
#line 2425 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-literal */
		}
		break;
	case (TOK_RANGE):
		{
			t_endpoint ZIr;
			t_char ZI191;
			t_pos ZI192;
			t_pos ZI193;
			t_endpoint ZIupper;
			t_pos ZIend;
			t_endpoint ZIlower;

			/* BEGINNING OF ACTION: ast-range-endpoint-literal */
			{
#line 626 "src/libre/parser.act"

		(ZIr).type = AST_ENDPOINT_LITERAL;
		(ZIr).u.literal.c = (*ZIcbrak);
	
#line 2447 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 241 "src/libre/parser.act"

		ZI191 = '-';
		ZI192 = lex_state->lx.start;
		ZI193   = lex_state->lx.end;
	
#line 2458 "src/libre/dialect/pcre/parser.c"
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
#line 626 "src/libre/parser.act"

		(ZIlower).type = AST_ENDPOINT_LITERAL;
		(ZIlower).u.literal.c = (*ZIcbrak);
	
#line 2474 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
			/* BEGINNING OF ACTION: ast-make-range */
			{
#line 786 "src/libre/parser.act"

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

		(ZInode1) = ast_make_expr_range(&(ZIlower), ast_start, &(ZIupper), ast_end);
		if ((ZInode1) == NULL) {
			goto ZL1;
		}
	
#line 2512 "src/libre/dialect/pcre/parser.c"
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
p_expr_C_Cpiece(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__expr ZIe;

		p_expr_C_Cpiece_C_Catom (flags, lex_state, act_state, err, &ZIe);
		/* BEGINNING OF INLINE: 238 */
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
#line 602 "src/libre/parser.act"

		(ZIc) = ast_make_count(1, NULL, 1, NULL);
	
#line 2562 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: count-one */
					/* BEGINNING OF ACTION: ast-make-piece */
					{
#line 696 "src/libre/parser.act"

		if ((ZIc).low == 0 && (ZIc).high == 0) {
			(ZInode) = ast_make_expr_empty();
		} else if ((ZIc).low == 1 && (ZIc).high == 1) {
			(ZInode) = (ZIe);
		} else {
			(ZInode) = ast_make_expr_repeat((ZIe), (ZIc));
		}
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL1;
		}
	
#line 2581 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-make-piece */
				}
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 238 */
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
#line 671 "src/libre/parser.act"

		(ZInode) = ast_make_expr_alt();
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2619 "src/libre/dialect/pcre/parser.c"
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
#line 490 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXALTS;
		}
		goto ZL2;
	
#line 2640 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: err-expected-alts */
		/* BEGINNING OF ACTION: ast-make-empty */
		{
#line 657 "src/libre/parser.act"

		(ZInode) = ast_make_expr_empty();
		if ((ZInode) == NULL) {
			goto ZL2;
		}
	
#line 2652 "src/libre/dialect/pcre/parser.c"
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
p_194(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZItmp)
{
ZL2_194:;
	switch (CURRENT_TERMINAL) {
	case (TOK_NAMED__CLASS): case (TOK_ESC): case (TOK_CONTROL): case (TOK_OCT):
	case (TOK_HEX): case (TOK_CHAR):
		{
			/* BEGINNING OF INLINE: expr::character-class::list-of-class-terms */
			{
				{
					/* BEGINNING OF INLINE: 156 */
					{
						{
							t_ast__expr ZInode;

							p_expr_C_Ccharacter_Hclass_C_Cclass_Hterm (flags, lex_state, act_state, err, &ZInode);
							if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
								RESTORE_LEXER;
								goto ZL5;
							}
							/* BEGINNING OF ACTION: ast-add-alt */
							{
#line 829 "src/libre/parser.act"

		if (!ast_add_expr_alt((*ZItmp), (ZInode))) {
			goto ZL5;
		}
	
#line 2693 "src/libre/dialect/pcre/parser.c"
							}
							/* END OF ACTION: ast-add-alt */
						}
						goto ZL4;
					ZL5:;
						{
							/* BEGINNING OF ACTION: err-expected-term */
							{
#line 469 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXTERM;
		}
		goto ZL1;
	
#line 2709 "src/libre/dialect/pcre/parser.c"
							}
							/* END OF ACTION: err-expected-term */
						}
					ZL4:;
					}
					/* END OF INLINE: 156 */
					/* BEGINNING OF INLINE: 194 */
					goto ZL2_194;
					/* END OF INLINE: 194 */
				}
				/*UNREACHED*/
			}
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
#line 578 "src/libre/parser.act"

		(ZIempty__pos) = RE_FLAGS_NONE;
	
#line 2763 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: re-flag-none */
		/* BEGINNING OF ACTION: re-flag-none */
		{
#line 578 "src/libre/parser.act"

		(ZIempty__neg) = RE_FLAGS_NONE;
	
#line 2772 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: re-flag-none */
		/* BEGINNING OF INLINE: 210 */
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
		/* END OF INLINE: 210 */
		/* BEGINNING OF INLINE: 212 */
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
		/* END OF INLINE: 212 */
		/* BEGINNING OF INLINE: 215 */
		{
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_CLOSEFLAGS):
					break;
				default:
					goto ZL5;
				}
				ADVANCE_LEXER;
				/* BEGINNING OF ACTION: ast-make-re-flags */
				{
#line 717 "src/libre/parser.act"

		(ZInode) = ast_make_expr_re_flags((ZIpos), (ZIneg));
		if ((ZInode) == NULL) {
			goto ZL5;
		}
	
#line 2835 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: ast-make-re-flags */
			}
			goto ZL4;
		ZL5:;
			{
				/* BEGINNING OF ACTION: err-expected-closeflags */
				{
#line 525 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCLOSEFLAGS;
		}
		goto ZL1;
	
#line 2851 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-closeflags */
				/* BEGINNING OF ACTION: ast-make-empty */
				{
#line 657 "src/libre/parser.act"

		(ZInode) = ast_make_expr_empty();
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 2863 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: ast-make-empty */
			}
		ZL4:;
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
#line 696 "src/libre/parser.act"

		if ((ZIc).low == 0 && (ZIc).high == 0) {
			(ZInode) = ast_make_expr_empty();
		} else if ((ZIc).low == 1 && (ZIc).high == 1) {
			(ZInode) = (ZIe);
		} else {
			(ZInode) = ast_make_expr_repeat((ZIe), (ZIc));
		}
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL1;
		}
	
#line 2911 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-make-piece */
		/* BEGINNING OF INLINE: 237 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_OPT): case (TOK_PLUS): case (TOK_STAR): case (TOK_OPENCOUNT):
				{
					p_expr_C_Cpiece_C_Clist_Hof_Hcounts (flags, lex_state, act_state, err, ZInode, &ZInode);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				break;
			}
		}
		/* END OF INLINE: 237 */
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
		/* BEGINNING OF INLINE: 135 */
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
		/* END OF INLINE: 135 */
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
#line 441 "src/libre/parser.act"

		ZIid = DIALECT_CLASS(lex_state->buf.a);
		if (ZIid == NULL) {
			/* syntax error -- unrecognized class */
			goto ZL1;
		}

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 3017 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: NAMED_CLASS */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: ast-make-named */
		{
#line 816 "src/libre/parser.act"

		(ZInode) = ast_make_expr_named((ZIid));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 3034 "src/libre/dialect/pcre/parser.c"
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
#line 829 "src/libre/parser.act"

		if (!ast_add_expr_alt((ZIalts), (ZIa))) {
			goto ZL1;
		}
	
#line 3071 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-add-alt */
		/* BEGINNING OF INLINE: 249 */
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
		/* END OF INLINE: 249 */
	}
	return;
ZL1:;
	{
		/* BEGINNING OF ACTION: err-expected-alts */
		{
#line 490 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXALTS;
		}
		goto ZL4;
	
#line 3103 "src/libre/dialect/pcre/parser.c"
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
			t_pos ZI286;
			t_pos ZI287;
			t_unsigned ZIm;

			/* BEGINNING OF EXTRACT: OPENCOUNT */
			{
#line 273 "src/libre/parser.act"

		ZI286 = lex_state->lx.start;
		ZI287   = lex_state->lx.end;
	
#line 3133 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: OPENCOUNT */
			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (TOK_COUNT):
				/* BEGINNING OF EXTRACT: COUNT */
				{
#line 429 "src/libre/parser.act"

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
	
#line 3161 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF EXTRACT: COUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			p_288 (flags, lex_state, act_state, err, &ZI286, &ZIm, &ZIc);
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
#line 598 "src/libre/parser.act"

		(ZIc) = ast_make_count(0, NULL, 1, NULL);
	
#line 3185 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: count-zero-or-one */
		}
		break;
	case (TOK_PLUS):
		{
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: count-one-or-more */
			{
#line 594 "src/libre/parser.act"

		(ZIc) = ast_make_count(1, NULL, AST_COUNT_UNBOUNDED, NULL);
	
#line 3199 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: count-one-or-more */
		}
		break;
	case (TOK_STAR):
		{
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: count-zero-or-more */
			{
#line 590 "src/libre/parser.act"

		(ZIc) = ast_make_count(0, NULL, AST_COUNT_UNBOUNDED, NULL);
	
#line 3213 "src/libre/dialect/pcre/parser.c"
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
#line 476 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCOUNT;
		}
		goto ZL2;
	
#line 3235 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: err-expected-count */
		/* BEGINNING OF ACTION: count-one */
		{
#line 602 "src/libre/parser.act"

		(ZIc) = ast_make_count(1, NULL, 1, NULL);
	
#line 3244 "src/libre/dialect/pcre/parser.c"
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
		/* BEGINNING OF INLINE: 251 */
		{
			{
				p_expr (flags, lex_state, act_state, err, &ZInode);
				if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
					RESTORE_LEXER;
					goto ZL1;
				}
			}
		}
		/* END OF INLINE: 251 */
		/* BEGINNING OF INLINE: 252 */
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
#line 532 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXEOF;
		}
		goto ZL1;
	
#line 3299 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL3:;
		}
		/* END OF INLINE: 252 */
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
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-make-any */
			{
#line 689 "src/libre/parser.act"

		(ZIe) = ast_make_expr_any();
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 3333 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-any */
		}
		break;
	case (TOK_CONTROL):
		{
			t_char ZI221;
			t_pos ZI222;
			t_pos ZI223;

			/* BEGINNING OF EXTRACT: CONTROL */
			{
#line 320 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] == 'c');
		assert(lex_state->buf.a[2] != '\0');
		assert(lex_state->buf.a[3] == '\0');

		ZI221 = lex_state->buf.a[2];
		if ((unsigned char) ZI221 > 127) {
			goto ZL1;
		}
		ZI221 = (((toupper((unsigned char)ZI221)) - 64) % 128 + 128) % 128;

		ZI222 = lex_state->lx.start;
		ZI223   = lex_state->lx.end;
	
#line 3362 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: CONTROL */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: err-unsupported */
			{
#line 539 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXUNSUPPORTD;
		}
		goto ZL1;
	
#line 3375 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: err-unsupported */
			/* BEGINNING OF ACTION: ast-make-empty */
			{
#line 657 "src/libre/parser.act"

		(ZIe) = ast_make_expr_empty();
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 3387 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-empty */
		}
		break;
	case (TOK_END):
		{
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-make-anchor-end */
			{
#line 731 "src/libre/parser.act"

		(ZIe) = ast_make_expr_anchor(AST_ANCHOR_END);
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 3404 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-make-anchor-end */
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
			/* BEGINNING OF ACTION: ast-make-group */
			{
#line 710 "src/libre/parser.act"

		(ZIe) = ast_make_expr_group((ZIg));
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 3428 "src/libre/dialect/pcre/parser.c"
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
	case (TOK_OPENSUB):
		{
			ADVANCE_LEXER;
			p_expr (flags, lex_state, act_state, err, &ZIe);
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSE):
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
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
#line 724 "src/libre/parser.act"

		(ZIe) = ast_make_expr_anchor(AST_ANCHOR_START);
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 3468 "src/libre/dialect/pcre/parser.c"
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
#line 483 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXATOM;
		}
		goto ZL2;
	
#line 3526 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: err-expected-atom */
		/* BEGINNING OF ACTION: ast-make-any */
		{
#line 689 "src/libre/parser.act"

		(ZIe) = ast_make_expr_any();
		if ((ZIe) == NULL) {
			goto ZL2;
		}
	
#line 3538 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-make-any */
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
	case (TOK_ANY): case (TOK_START): case (TOK_END): case (TOK_OPENSUB):
	case (TOK_OPENCAPTURE): case (TOK_OPENGROUP): case (TOK_OPENGROUPINV): case (TOK_OPENGROUPCB):
	case (TOK_OPENGROUPINVCB): case (TOK_NAMED__CLASS): case (TOK_OPENFLAGS): case (TOK_ESC):
	case (TOK_CONTROL): case (TOK_OCT): case (TOK_HEX): case (TOK_CHAR):
		{
			/* BEGINNING OF ACTION: ast-make-concat */
			{
#line 664 "src/libre/parser.act"

		(ZInode) = ast_make_expr_concat();
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 3570 "src/libre/dialect/pcre/parser.c"
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
#line 657 "src/libre/parser.act"

		(ZInode) = ast_make_expr_empty();
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 3591 "src/libre/dialect/pcre/parser.c"
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
#line 671 "src/libre/parser.act"

		(ZInode) = ast_make_expr_alt();
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 3634 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-make-alt */
		/* BEGINNING OF ACTION: ast-add-alt */
		{
#line 829 "src/libre/parser.act"

		if (!ast_add_expr_alt((ZInode), (ZIclass))) {
			goto ZL1;
		}
	
#line 3645 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-add-alt */
		/* BEGINNING OF ACTION: mark-expr */
		{
#line 560 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;

		AST_POS_OF_LX_POS(ast_start, (ZIstart));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));

/* TODO: reinstate this, applies to an expr node in general
		(ZInode)->u.class.start = ast_start;
		(ZInode)->u.class.end   = ast_end;
*/
	
#line 3665 "src/libre/dialect/pcre/parser.c"
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

#line 976 "src/libre/parser.act"


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

#line 3822 "src/libre/dialect/pcre/parser.c"

/* END OF FILE */
