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
	#include <stdlib.h>
	#include <stdio.h>
	#include <errno.h>
	#include <ctype.h>

	#include "libre/re_ast.h"
	#include "libre/re_char_class.h"

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

	#define DIALECT_PARSE  CAT(parse_re_, DIALECT)
	#define DIALECT_CHAR_CLASS  CAT(re_char_class_, DIALECT)
	#define DIALECT_CHAR_TYPE   CAT(re_char_type_, DIALECT)

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
	#define AST_POS_OF_LX_POS(AST_POS, LX_POS)			\
		do {							\
			AST_POS.line = LX_POS.line;			\
			AST_POS.col = LX_POS.col;			\
			AST_POS.byte = LX_POS.byte;			\
		} while (0)


	#include "parser.h"
	#include "lexer.h"

	#include "../comp.h"
	#include "../../class.h"
	#include "../../re_char_class.h"

	struct flags {
		enum re_flags flags;
		struct flags *parent;
	};

	typedef char     t_char;
	typedef unsigned t_unsigned;
	typedef unsigned t_pred; /* TODO */

	typedef struct lx_pos t_pos;
	typedef enum re_flags t_re__flags;
	typedef char_class_constructor_fun * t_ast__char__class__id;
	typedef struct ast_count t_ast__count;

	typedef struct re_char_class_ast * t_char__class__ast;
	typedef struct ast_char_class * t_char__class;
	typedef enum re_char_class_flags t_char__class__flags;

	typedef struct ast_range_endpoint t_range__endpoint;

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

#line 219 "src/libre/dialect/pcre/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_254(flags, lex_state, act_state, err, t_ast__expr *, t_ast__expr *);
static void p_expr_C_Cflags_C_Cflag__set(flags, lex_state, act_state, err, t_re__flags, t_re__flags *);
static void p_141(flags, lex_state, act_state, err);
static void p_145(flags, lex_state, act_state, err, t_range__endpoint *, t_pos *);
static void p_expr_C_Cchar_Hclass_C_Cclass_Hterm(flags, lex_state, act_state, err, t_char__class__ast *);
static void p_expr_C_Cchar_Hclass_C_Cclass_Hrange_C_Crange_Hendpoint_Hchar_Hclass(flags, lex_state, act_state, err, t_range__endpoint *, t_pos *, t_pos *);
static void p_expr_C_Cchar_Hclass(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Cchar_Hclass_C_Cclass_Hrange_C_Crange_Hendpoint_Hliteral(flags, lex_state, act_state, err, t_range__endpoint *, t_pos *, t_pos *);
static void p_expr_C_Cliteral(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Cchar_Hclass_C_Cchar_Hclass_Hhead(flags, lex_state, act_state, err, t_char__class__ast *);
static void p_expr(flags, lex_state, act_state, err, t_ast__expr *);
static void p_194(flags, lex_state, act_state, err);
static void p_expr_C_Cflags(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Cchar_Hclass_C_Clist_Hof_Hchar_Hclass_Hterms(flags, lex_state, act_state, err, t_char__class__ast *);
static void p_expr_C_Cchar_Htype(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Clist_Hof_Hatoms(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Clist_Hof_Halts(flags, lex_state, act_state, err, t_ast__expr *);
static void p_225(flags, lex_state, act_state, err, t_ast__expr *, t_ast__expr *);
static void p_226(flags, lex_state, act_state, err, t_ast__expr *, t_ast__expr *);
extern void p_re__pcre(flags, lex_state, act_state, err, t_ast__expr *);
static void p_230(flags, lex_state, act_state, err, t_ast__char__class__id *, t_pos *, t_char__class__ast *);
static void p_expr_C_Catom(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Calt(flags, lex_state, act_state, err, t_ast__expr *);
static void p_250(flags, lex_state, act_state, err, t_char *, t_pos *, t_char__class__ast *);
static void p_253(flags, lex_state, act_state, err, t_ast__expr *, t_pos *, t_unsigned *, t_ast__expr *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_254(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZIa, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_ALT):
		{
			t_ast__expr ZIr;

			ADVANCE_LEXER;
			p_expr_C_Clist_Hof_Halts (flags, lex_state, act_state, err, &ZIr);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: ast-expr-alt */
			{
#line 569 "src/libre/parser.act"

		(ZInode) = re_ast_expr_alt((*ZIa), (ZIr));
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 282 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-expr-alt */
		}
		break;
	default:
		{
			t_ast__expr ZIr;

			/* BEGINNING OF ACTION: ast-expr-empty */
			{
#line 559 "src/libre/parser.act"

		(ZIr) = re_ast_expr_empty();
		if ((ZIr) == NULL) { goto ZL1; }
	
#line 298 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-expr-empty */
			/* BEGINNING OF ACTION: ast-expr-alt */
			{
#line 569 "src/libre/parser.act"

		(ZInode) = re_ast_expr_alt((*ZIa), (ZIr));
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 308 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-expr-alt */
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
	
#line 340 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: FLAG_INSENSITIVE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: re-flag-union */
			{
#line 753 "src/libre/parser.act"

		(ZIo) = (ZIi) | (ZIc);
	
#line 350 "src/libre/dialect/pcre/parser.c"
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
#line 514 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EFLAG;
		}
		goto ZL1;
	
#line 368 "src/libre/dialect/pcre/parser.c"
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
p_141(flags flags, lex_state lex_state, act_state act_state, err err)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZI142;
		t_pos ZI143;
		t_pos ZI144;

		switch (CURRENT_TERMINAL) {
		case (TOK_RANGE):
			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 246 "src/libre/parser.act"

		ZI142 = '-';
		ZI143 = lex_state->lx.start;
		ZI144   = lex_state->lx.end;
	
#line 407 "src/libre/dialect/pcre/parser.c"
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
#line 493 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXRANGE;
		}
		goto ZL2;
	
#line 428 "src/libre/dialect/pcre/parser.c"
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
p_145(flags flags, lex_state lex_state, act_state act_state, err err, t_range__endpoint *ZOupper, t_pos *ZOu__end)
{
	t_range__endpoint ZIupper;
	t_pos ZIu__end;

	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZIc;
			t_pos ZIu__start;

			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 246 "src/libre/parser.act"

		ZIc = '-';
		ZIu__start = lex_state->lx.start;
		ZIu__end   = lex_state->lx.end;
	
#line 459 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-range-endpoint-literal */
			{
#line 735 "src/libre/parser.act"

		struct ast_range_endpoint range;
		range.t = AST_RANGE_ENDPOINT_LITERAL;
		range.u.literal.c = (ZIc);
		(ZIupper) = range;
	
#line 472 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
		}
		break;
	case (TOK_NAMED__CHAR__CLASS):
		{
			t_pos ZIu__start;

			p_expr_C_Cchar_Hclass_C_Cclass_Hrange_C_Crange_Hendpoint_Hchar_Hclass (flags, lex_state, act_state, err, &ZIupper, &ZIu__start, &ZIu__end);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_ESC): case (TOK_CONTROL): case (TOK_OCT): case (TOK_HEX):
	case (TOK_CHAR):
		{
			t_pos ZIu__start;

			p_expr_C_Cchar_Hclass_C_Cclass_Hrange_C_Crange_Hendpoint_Hliteral (flags, lex_state, act_state, err, &ZIupper, &ZIu__start, &ZIu__end);
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
	*ZOu__end = ZIu__end;
}

static void
p_expr_C_Cchar_Hclass_C_Cclass_Hterm(flags flags, lex_state lex_state, act_state act_state, err err, t_char__class__ast *ZOnode)
{
	t_char__class__ast ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_CHAR):
		{
			t_char ZI243;
			t_pos ZI244;
			t_pos ZI245;

			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 401 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI244 = lex_state->lx.start;
		ZI245   = lex_state->lx.end;

		ZI243 = lex_state->buf.a[0];
	
#line 538 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			ADVANCE_LEXER;
			p_250 (flags, lex_state, act_state, err, &ZI243, &ZI244, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_CONTROL):
		{
			t_char ZI247;
			t_pos ZI248;
			t_pos ZI249;

			/* BEGINNING OF EXTRACT: CONTROL */
			{
#line 305 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] == 'c');
		assert(lex_state->buf.a[2] != '\0');
		assert(lex_state->buf.a[3] == '\0');

		ZI247 = lex_state->buf.a[2];
		if ((unsigned char) ZI247 > 127) {
			goto ZL1;
		}
		ZI247 = (((toupper((unsigned char)ZI247)) - 64) % 128 + 128) % 128;

		ZI248 = lex_state->lx.start;
		ZI249   = lex_state->lx.end;
	
#line 573 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: CONTROL */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: err-unsupported */
			{
#line 535 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXUNSUPPORTD;
		}
		goto ZL1;
	
#line 586 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: err-unsupported */
			p_250 (flags, lex_state, act_state, err, &ZI247, &ZI248, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_ESC):
		{
			t_char ZI231;
			t_pos ZI232;
			t_pos ZI233;

			/* BEGINNING OF EXTRACT: ESC */
			{
#line 273 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] != '\0');
		assert(lex_state->buf.a[2] == '\0');

		ZI231 = lex_state->buf.a[1];

		switch (ZI231) {
		case 'a': ZI231 = '\a'; break;
		case 'f': ZI231 = '\f'; break;
		case 'n': ZI231 = '\n'; break;
		case 'r': ZI231 = '\r'; break;
		case 't': ZI231 = '\t'; break;
		case 'v': ZI231 = '\v'; break;
		default:             break;
		}

		ZI232 = lex_state->lx.start;
		ZI233   = lex_state->lx.end;
	
#line 625 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: ESC */
			ADVANCE_LEXER;
			p_250 (flags, lex_state, act_state, err, &ZI231, &ZI232, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_HEX):
		{
			t_char ZI239;
			t_pos ZI240;
			t_pos ZI241;

			/* BEGINNING OF EXTRACT: HEX */
			{
#line 361 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\x", 2));
		assert(strlen(lex_state->buf.a) >= 3);

		ZI240 = lex_state->lx.start;
		ZI241   = lex_state->lx.end;

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

		ZI239 = (char) (unsigned char) u;
	
#line 684 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: HEX */
			ADVANCE_LEXER;
			p_250 (flags, lex_state, act_state, err, &ZI239, &ZI240, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_OCT):
		{
			t_char ZI235;
			t_pos ZI236;
			t_pos ZI237;

			/* BEGINNING OF EXTRACT: OCT */
			{
#line 321 "src/libre/parser.act"

		unsigned long u;
		char *s, *e;
		int brace = 0;

		assert(0 == strncmp(lex_state->buf.a, "\\", 1));
		assert(strlen(lex_state->buf.a) >= 2);

		ZI236 = lex_state->lx.start;
		ZI237   = lex_state->lx.end;

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

		ZI235 = (char) (unsigned char) u;
	
#line 743 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: OCT */
			ADVANCE_LEXER;
			p_250 (flags, lex_state, act_state, err, &ZI235, &ZI236, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_NAMED__CHAR__CLASS):
		{
			t_ast__char__class__id ZI227;
			t_pos ZI228;
			t_pos ZI229;

			/* BEGINNING OF INLINE: expr::char-class::class-named */
			{
				{
					switch (CURRENT_TERMINAL) {
					case (TOK_NAMED__CHAR__CLASS):
						/* BEGINNING OF EXTRACT: NAMED_CHAR_CLASS */
						{
#line 431 "src/libre/parser.act"

		enum re_dialect_char_class_lookup_res res;
		res = DIALECT_CHAR_CLASS(lex_state->buf.a, &ZI227);

		switch (res) {
		default:
		case RE_CLASS_NOT_FOUND:
			/* syntax error -- unrecognized class */
			goto ZL1;
		case RE_CLASS_UNSUPPORTED:
			err->e = RE_EXUNSUPPORTD;
			goto ZL1;
		case RE_CLASS_FOUND:
			/* proceed below */
			break;
		}

		ZI228 = lex_state->lx.start;
		ZI229   = lex_state->lx.end;
	
#line 788 "src/libre/dialect/pcre/parser.c"
						}
						/* END OF EXTRACT: NAMED_CHAR_CLASS */
						break;
					default:
						goto ZL1;
					}
					ADVANCE_LEXER;
				}
			}
			/* END OF INLINE: expr::char-class::class-named */
			p_230 (flags, lex_state, act_state, err, &ZI227, &ZI228, &ZInode);
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
		/* BEGINNING OF ACTION: err-expected-term */
		{
#line 465 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXTERM;
		}
		goto ZL3;
	
#line 823 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: err-expected-term */
		/* BEGINNING OF ACTION: char-class-ast-flag-none */
		{
#line 715 "src/libre/parser.act"

		(ZInode) = re_char_class_ast_flags(RE_CHAR_CLASS_FLAG_NONE);
		if ((ZInode) == NULL) { goto ZL3; }
	
#line 833 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: char-class-ast-flag-none */
	}
	goto ZL0;
ZL3:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Cchar_Hclass_C_Cclass_Hrange_C_Crange_Hendpoint_Hchar_Hclass(flags flags, lex_state lex_state, act_state act_state, err err, t_range__endpoint *ZOr, t_pos *ZOstart, t_pos *ZOend)
{
	t_range__endpoint ZIr;
	t_pos ZIstart;
	t_pos ZIend;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__char__class__id ZIid;

		/* BEGINNING OF INLINE: expr::char-class::class-named */
		{
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_NAMED__CHAR__CLASS):
					/* BEGINNING OF EXTRACT: NAMED_CHAR_CLASS */
					{
#line 431 "src/libre/parser.act"

		enum re_dialect_char_class_lookup_res res;
		res = DIALECT_CHAR_CLASS(lex_state->buf.a, &ZIid);

		switch (res) {
		default:
		case RE_CLASS_NOT_FOUND:
			/* syntax error -- unrecognized class */
			goto ZL1;
		case RE_CLASS_UNSUPPORTED:
			err->e = RE_EXUNSUPPORTD;
			goto ZL1;
		case RE_CLASS_FOUND:
			/* proceed below */
			break;
		}

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 886 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: NAMED_CHAR_CLASS */
					break;
				default:
					goto ZL1;
				}
				ADVANCE_LEXER;
			}
		}
		/* END OF INLINE: expr::char-class::class-named */
		/* BEGINNING OF ACTION: ast-range-endpoint-char-class */
		{
#line 742 "src/libre/parser.act"

		struct ast_range_endpoint range;
		range.t = AST_RANGE_ENDPOINT_CHAR_CLASS;
		range.u.char_class.ctor = (ZIid);
		(ZIr) = range;
	
#line 906 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-range-endpoint-char-class */
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
p_expr_C_Cchar_Hclass(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_pos ZIstart;
		t_pos ZI162;
		t_char__class__ast ZIhead;
		t_char__class__ast ZIbody;
		t_pos ZIend;
		t_char__class__ast ZIhb;

		switch (CURRENT_TERMINAL) {
		case (TOK_OPENGROUP):
			/* BEGINNING OF EXTRACT: OPENGROUP */
			{
#line 252 "src/libre/parser.act"

		ZIstart = lex_state->lx.start;
		ZI162   = lex_state->lx.end;
	
#line 945 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: OPENGROUP */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		p_expr_C_Cchar_Hclass_C_Cchar_Hclass_Hhead (flags, lex_state, act_state, err, &ZIhead);
		p_expr_C_Cchar_Hclass_C_Clist_Hof_Hchar_Hclass_Hterms (flags, lex_state, act_state, err, &ZIbody);
		/* BEGINNING OF INLINE: 165 */
		{
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			{
				t_char ZI166;
				t_pos ZI167;

				switch (CURRENT_TERMINAL) {
				case (TOK_CLOSEGROUP):
					/* BEGINNING OF EXTRACT: CLOSEGROUP */
					{
#line 257 "src/libre/parser.act"

		ZI166 = ']';
		ZI167 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 975 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CLOSEGROUP */
					break;
				default:
					goto ZL3;
				}
				ADVANCE_LEXER;
				/* BEGINNING OF ACTION: mark-group */
				{
#line 542 "src/libre/parser.act"

		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));
	
#line 990 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: mark-group */
			}
			goto ZL2;
		ZL3:;
			{
				/* BEGINNING OF ACTION: err-expected-closegroup */
				{
#line 500 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCLOSEGROUP;
		}
		goto ZL1;
	
#line 1006 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-closegroup */
				ZIend = ZIstart;
			}
		ZL2:;
		}
		/* END OF INLINE: 165 */
		/* BEGINNING OF ACTION: char-class-ast-concat */
		{
#line 690 "src/libre/parser.act"

		(ZIhb) = re_char_class_ast_concat((ZIhead), (ZIbody));
		if ((ZIhb) == NULL) { goto ZL1; }
	
#line 1021 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: char-class-ast-concat */
		/* BEGINNING OF ACTION: ast-expr-char-class */
		{
#line 705 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		AST_POS_OF_LX_POS(ast_start, (ZIstart));
		AST_POS_OF_LX_POS(ast_end, (ZIend));
		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));
		(ZInode) = re_ast_expr_char_class((ZIhb), &ast_start, &ast_end);
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 1036 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-expr-char-class */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Cchar_Hclass_C_Cclass_Hrange_C_Crange_Hendpoint_Hliteral(flags flags, lex_state lex_state, act_state act_state, err err, t_range__endpoint *ZOr, t_pos *ZOstart, t_pos *ZOend)
{
	t_range__endpoint ZIr;
	t_pos ZIstart;
	t_pos ZIend;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZIc;

		/* BEGINNING OF INLINE: 133 */
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
	
#line 1078 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CHAR */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_CONTROL):
				{
					/* BEGINNING OF EXTRACT: CONTROL */
					{
#line 305 "src/libre/parser.act"

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
	
#line 1104 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CONTROL */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: err-unsupported */
					{
#line 535 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXUNSUPPORTD;
		}
		goto ZL1;
	
#line 1117 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: err-unsupported */
				}
				break;
			case (TOK_ESC):
				{
					/* BEGINNING OF EXTRACT: ESC */
					{
#line 273 "src/libre/parser.act"

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
	
#line 1147 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: ESC */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_HEX):
				{
					/* BEGINNING OF EXTRACT: HEX */
					{
#line 361 "src/libre/parser.act"

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
	
#line 1197 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: HEX */
					ADVANCE_LEXER;
				}
				break;
			case (TOK_OCT):
				{
					/* BEGINNING OF EXTRACT: OCT */
					{
#line 321 "src/libre/parser.act"

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
	
#line 1247 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OCT */
					ADVANCE_LEXER;
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 133 */
		/* BEGINNING OF ACTION: ast-range-endpoint-literal */
		{
#line 735 "src/libre/parser.act"

		struct ast_range_endpoint range;
		range.t = AST_RANGE_ENDPOINT_LITERAL;
		range.u.literal.c = (ZIc);
		(ZIr) = range;
	
#line 1267 "src/libre/dialect/pcre/parser.c"
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
#line 401 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI102 = lex_state->lx.start;
		ZI103   = lex_state->lx.end;

		ZIc = lex_state->buf.a[0];
	
#line 1312 "src/libre/dialect/pcre/parser.c"
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
#line 273 "src/libre/parser.act"

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
	
#line 1346 "src/libre/dialect/pcre/parser.c"
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
#line 361 "src/libre/parser.act"

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
	
#line 1399 "src/libre/dialect/pcre/parser.c"
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
#line 321 "src/libre/parser.act"

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
	
#line 1452 "src/libre/dialect/pcre/parser.c"
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
		/* BEGINNING OF ACTION: ast-expr-literal */
		{
#line 574 "src/libre/parser.act"

		(ZInode) = re_ast_expr_literal((ZIc));
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 1470 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-expr-literal */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Cchar_Hclass_C_Cchar_Hclass_Hhead(flags flags, lex_state lex_state, act_state act_state, err err, t_char__class__ast *ZOf)
{
	t_char__class__ast ZIf;

	switch (CURRENT_TERMINAL) {
	case (TOK_INVERT):
		{
			t_char ZI109;

			/* BEGINNING OF EXTRACT: INVERT */
			{
#line 242 "src/libre/parser.act"

		ZI109 = '^';
	
#line 1498 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: INVERT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: char-class-ast-flag-invert */
			{
#line 720 "src/libre/parser.act"

		(ZIf) = re_char_class_ast_flags(RE_CHAR_CLASS_FLAG_INVERTED);
		if ((ZIf) == NULL) { goto ZL1; }
	
#line 1509 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: char-class-ast-flag-invert */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: char-class-ast-flag-none */
			{
#line 715 "src/libre/parser.act"

		(ZIf) = re_char_class_ast_flags(RE_CHAR_CLASS_FLAG_NONE);
		if ((ZIf) == NULL) { goto ZL1; }
	
#line 1523 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: char-class-ast-flag-none */
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
	*ZOf = ZIf;
}

static void
p_expr(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__expr ZI223;

		p_expr_C_Calt (flags, lex_state, act_state, err, &ZI223);
		p_225 (flags, lex_state, act_state, err, &ZI223, &ZInode);
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
#line 486 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXALTS;
		}
		goto ZL2;
	
#line 1569 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: err-expected-alts */
		/* BEGINNING OF ACTION: ast-expr-empty */
		{
#line 559 "src/libre/parser.act"

		(ZInode) = re_ast_expr_empty();
		if ((ZInode) == NULL) { goto ZL2; }
	
#line 1579 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-expr-empty */
	}
	goto ZL0;
ZL2:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_194(flags flags, lex_state lex_state, act_state act_state, err err)
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
#line 486 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXALTS;
		}
		goto ZL2;
	
#line 1618 "src/libre/dialect/pcre/parser.c"
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
#line 749 "src/libre/parser.act"

	        (ZIempty__pos) = RE_FLAGS_NONE;
	
#line 1656 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: re-flag-none */
		/* BEGINNING OF ACTION: re-flag-none */
		{
#line 749 "src/libre/parser.act"

	        (ZIempty__neg) = RE_FLAGS_NONE;
	
#line 1665 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: re-flag-none */
		/* BEGINNING OF INLINE: 182 */
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
		/* END OF INLINE: 182 */
		/* BEGINNING OF INLINE: 184 */
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
		/* END OF INLINE: 184 */
		/* BEGINNING OF INLINE: 187 */
		{
			{
				switch (CURRENT_TERMINAL) {
				case (TOK_CLOSEFLAGS):
					break;
				default:
					goto ZL5;
				}
				ADVANCE_LEXER;
				/* BEGINNING OF ACTION: ast-expr-re-flags */
				{
#line 757 "src/libre/parser.act"

		(ZInode) = re_ast_expr_re_flags((ZIpos), (ZIneg));
		if ((ZInode) == NULL) { goto ZL5; }
	
#line 1726 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: ast-expr-re-flags */
			}
			goto ZL4;
		ZL5:;
			{
				/* BEGINNING OF ACTION: err-expected-closeflags */
				{
#line 521 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCLOSEFLAGS;
		}
		goto ZL1;
	
#line 1742 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-closeflags */
				/* BEGINNING OF ACTION: ast-expr-empty */
				{
#line 559 "src/libre/parser.act"

		(ZInode) = re_ast_expr_empty();
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 1752 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: ast-expr-empty */
			}
		ZL4:;
		}
		/* END OF INLINE: 187 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Cchar_Hclass_C_Clist_Hof_Hchar_Hclass_Hterms(flags flags, lex_state lex_state, act_state act_state, err err, t_char__class__ast *ZOnode)
{
	t_char__class__ast ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char__class__ast ZIl;

		p_expr_C_Cchar_Hclass_C_Cclass_Hterm (flags, lex_state, act_state, err, &ZIl);
		/* BEGINNING OF INLINE: 160 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_NAMED__CHAR__CLASS): case (TOK_ESC): case (TOK_CONTROL): case (TOK_OCT):
			case (TOK_HEX): case (TOK_CHAR):
				{
					t_char__class__ast ZIr;

					p_expr_C_Cchar_Hclass_C_Clist_Hof_Hchar_Hclass_Hterms (flags, lex_state, act_state, err, &ZIr);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
					/* BEGINNING OF ACTION: char-class-ast-concat */
					{
#line 690 "src/libre/parser.act"

		(ZInode) = re_char_class_ast_concat((ZIl), (ZIr));
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 1800 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: char-class-ast-concat */
				}
				break;
			default:
				{
					ZInode = ZIl;
				}
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		/* END OF INLINE: 160 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Cchar_Htype(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__char__class__id ZIid;
		t_pos ZIstart;
		t_pos ZIend;
		t_char__class__ast ZInamed;

		switch (CURRENT_TERMINAL) {
		case (TOK_NAMED__CHAR__CLASS):
			/* BEGINNING OF EXTRACT: NAMED_CHAR_CLASS */
			{
#line 431 "src/libre/parser.act"

		enum re_dialect_char_class_lookup_res res;
		res = DIALECT_CHAR_CLASS(lex_state->buf.a, &ZIid);

		switch (res) {
		default:
		case RE_CLASS_NOT_FOUND:
			/* syntax error -- unrecognized class */
			goto ZL1;
		case RE_CLASS_UNSUPPORTED:
			err->e = RE_EXUNSUPPORTD;
			goto ZL1;
		case RE_CLASS_FOUND:
			/* proceed below */
			break;
		}

		ZIstart = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 1864 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: NAMED_CHAR_CLASS */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: char-class-ast-named-class */
		{
#line 700 "src/libre/parser.act"

		(ZInamed) = re_char_class_ast_named_class((ZIid));
		if ((ZInamed) == NULL) { goto ZL1; }
	
#line 1879 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: char-class-ast-named-class */
		/* BEGINNING OF ACTION: ast-expr-char-class */
		{
#line 705 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		AST_POS_OF_LX_POS(ast_start, (ZIstart));
		AST_POS_OF_LX_POS(ast_end, (ZIend));
		mark(&act_state->groupstart, &(ZIstart));
		mark(&act_state->groupend,   &(ZIend));
		(ZInode) = re_ast_expr_char_class((ZInamed), &ast_start, &ast_end);
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 1894 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-expr-char-class */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Clist_Hof_Hatoms(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__expr ZIa;

		p_expr_C_Catom (flags, lex_state, act_state, err, &ZIa);
		p_226 (flags, lex_state, act_state, err, &ZIa, &ZInode);
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
p_expr_C_Clist_Hof_Halts(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_ANY): case (TOK_START): case (TOK_END): case (TOK_OPENSUB):
	case (TOK_OPENCAPTURE): case (TOK_OPENGROUP): case (TOK_NAMED__CHAR__CLASS): case (TOK_OPENFLAGS):
	case (TOK_ESC): case (TOK_CONTROL): case (TOK_OCT): case (TOK_HEX):
	case (TOK_CHAR):
		{
			t_ast__expr ZIa;

			p_expr_C_Calt (flags, lex_state, act_state, err, &ZIa);
			p_254 (flags, lex_state, act_state, err, &ZIa, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	default:
		{
			t_ast__expr ZIa;
			t_ast__expr ZIr;

			/* BEGINNING OF ACTION: ast-expr-empty */
			{
#line 559 "src/libre/parser.act"

		(ZIa) = re_ast_expr_empty();
		if ((ZIa) == NULL) { goto ZL1; }
	
#line 1965 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-expr-empty */
			/* BEGINNING OF ACTION: ast-expr-empty */
			{
#line 559 "src/libre/parser.act"

		(ZIr) = re_ast_expr_empty();
		if ((ZIr) == NULL) { goto ZL1; }
	
#line 1975 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-expr-empty */
			/* BEGINNING OF ACTION: ast-expr-alt */
			{
#line 569 "src/libre/parser.act"

		(ZInode) = re_ast_expr_alt((ZIa), (ZIr));
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 1985 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-expr-alt */
		}
		break;
	case (ERROR_TERMINAL):
		return;
	}
	goto ZL0;
ZL1:;
	{
		/* BEGINNING OF ACTION: ast-expr-empty */
		{
#line 559 "src/libre/parser.act"

		(ZInode) = re_ast_expr_empty();
		if ((ZInode) == NULL) { goto ZL2; }
	
#line 2003 "src/libre/dialect/pcre/parser.c"
		}
		/* END OF ACTION: ast-expr-empty */
	}
	goto ZL0;
ZL2:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_225(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZI223, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_ALT):
		{
			t_ast__expr ZIr;

			ADVANCE_LEXER;
			p_expr_C_Clist_Hof_Halts (flags, lex_state, act_state, err, &ZIr);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: ast-expr-alt */
			{
#line 569 "src/libre/parser.act"

		(ZInode) = re_ast_expr_alt((*ZI223), (ZIr));
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 2038 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-expr-alt */
		}
		break;
	default:
		{
			ZInode = *ZI223;
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
p_226(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZIa, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_ANY): case (TOK_START): case (TOK_END): case (TOK_OPENSUB):
	case (TOK_OPENCAPTURE): case (TOK_OPENGROUP): case (TOK_NAMED__CHAR__CLASS): case (TOK_OPENFLAGS):
	case (TOK_ESC): case (TOK_CONTROL): case (TOK_OCT): case (TOK_HEX):
	case (TOK_CHAR):
		{
			t_ast__expr ZIr;

			p_expr_C_Clist_Hof_Hatoms (flags, lex_state, act_state, err, &ZIr);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: ast-expr-concat */
			{
#line 564 "src/libre/parser.act"

		(ZInode) = re_ast_expr_concat((*ZIa), (ZIr));
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 2084 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-expr-concat */
		}
		break;
	default:
		{
			t_ast__expr ZIr;

			/* BEGINNING OF ACTION: ast-expr-empty */
			{
#line 559 "src/libre/parser.act"

		(ZIr) = re_ast_expr_empty();
		if ((ZIr) == NULL) { goto ZL1; }
	
#line 2100 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-expr-empty */
			/* BEGINNING OF ACTION: ast-expr-concat */
			{
#line 564 "src/libre/parser.act"

		(ZInode) = re_ast_expr_concat((*ZIa), (ZIr));
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 2110 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-expr-concat */
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
#line 528 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXEOF;
		}
		goto ZL1;
	
#line 2169 "src/libre/dialect/pcre/parser.c"
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
p_230(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__char__class__id *ZI227, t_pos *ZI228, t_char__class__ast *ZOnode)
{
	t_char__class__ast ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_range__endpoint ZIlower;
			t_range__endpoint ZIupper;
			t_pos ZIu__end;
			t_pos ZI153;
			t_pos ZI154;

			/* BEGINNING OF ACTION: ast-range-endpoint-char-class */
			{
#line 742 "src/libre/parser.act"

		struct ast_range_endpoint range;
		range.t = AST_RANGE_ENDPOINT_CHAR_CLASS;
		range.u.char_class.ctor = (*ZI227);
		(ZIlower) = range;
	
#line 2208 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-char-class */
			p_141 (flags, lex_state, act_state, err);
			p_145 (flags, lex_state, act_state, err, &ZIupper, &ZIu__end);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: mark-range */
			{
#line 547 "src/libre/parser.act"

		mark(&act_state->rangestart, &(*ZI228));
		mark(&act_state->rangeend,   &(ZIu__end));
	
#line 2224 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-range */
			ZI153 = *ZI228;
			ZI154 = ZIu__end;
			/* BEGINNING OF ACTION: char-class-ast-range */
			{
#line 660 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		unsigned char lower, upper;
		AST_POS_OF_LX_POS(ast_start, (*ZI228));
		AST_POS_OF_LX_POS(ast_end, (ZIu__end));

		if ((ZIlower).t != AST_RANGE_ENDPOINT_LITERAL ||
		    (ZIupper).t != AST_RANGE_ENDPOINT_LITERAL) {
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

		(ZInode) = re_char_class_ast_range(&(ZIlower), ast_start, &(ZIupper), ast_end);
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 2261 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: char-class-ast-range */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: char-class-ast-named-class */
			{
#line 700 "src/libre/parser.act"

		(ZInode) = re_char_class_ast_named_class((*ZI227));
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 2275 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: char-class-ast-named-class */
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
p_expr_C_Catom(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__expr ZIe;

		/* BEGINNING OF INLINE: 190 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-expr-any */
					{
#line 579 "src/libre/parser.act"

		(ZIe) = re_ast_expr_any();
		if ((ZIe) == NULL) { goto ZL1; }
	
#line 2315 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-expr-any */
				}
				break;
			case (TOK_CONTROL):
				{
					t_char ZI195;
					t_pos ZI196;
					t_pos ZI197;

					/* BEGINNING OF EXTRACT: CONTROL */
					{
#line 305 "src/libre/parser.act"

		assert(lex_state->buf.a[0] == '\\');
		assert(lex_state->buf.a[1] == 'c');
		assert(lex_state->buf.a[2] != '\0');
		assert(lex_state->buf.a[3] == '\0');

		ZI195 = lex_state->buf.a[2];
		if ((unsigned char) ZI195 > 127) {
			goto ZL1;
		}
		ZI195 = (((toupper((unsigned char)ZI195)) - 64) % 128 + 128) % 128;

		ZI196 = lex_state->lx.start;
		ZI197   = lex_state->lx.end;
	
#line 2344 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: CONTROL */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: err-unsupported */
					{
#line 535 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXUNSUPPORTD;
		}
		goto ZL1;
	
#line 2357 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: err-unsupported */
					/* BEGINNING OF ACTION: ast-expr-empty */
					{
#line 559 "src/libre/parser.act"

		(ZIe) = re_ast_expr_empty();
		if ((ZIe) == NULL) { goto ZL1; }
	
#line 2367 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-expr-empty */
				}
				break;
			case (TOK_END):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-expr-anchor-end */
					{
#line 767 "src/libre/parser.act"

		(ZIe) = re_ast_expr_anchor(RE_AST_ANCHOR_END);
		if ((ZIe) == NULL) { goto ZL1; }
	
#line 2382 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-expr-anchor-end */
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
					/* BEGINNING OF ACTION: ast-expr-group */
					{
#line 591 "src/libre/parser.act"

		(ZIe) = re_ast_expr_group((ZIg));
		if ((ZIe) == NULL) { goto ZL1; }
	
#line 2404 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-expr-group */
					p_194 (flags, lex_state, act_state, err);
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
					p_194 (flags, lex_state, act_state, err);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_START):
				{
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-expr-anchor-start */
					{
#line 762 "src/libre/parser.act"

		(ZIe) = re_ast_expr_anchor(RE_AST_ANCHOR_START);
		if ((ZIe) == NULL) { goto ZL1; }
	
#line 2435 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-expr-anchor-start */
				}
				break;
			case (TOK_OPENGROUP):
				{
					p_expr_C_Cchar_Hclass (flags, lex_state, act_state, err, &ZIe);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			case (TOK_NAMED__CHAR__CLASS):
				{
					p_expr_C_Cchar_Htype (flags, lex_state, act_state, err, &ZIe);
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
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 190 */
		/* BEGINNING OF INLINE: 198 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_OPENCOUNT):
				{
					t_pos ZI251;
					t_pos ZI252;
					t_unsigned ZIm;

					/* BEGINNING OF EXTRACT: OPENCOUNT */
					{
#line 263 "src/libre/parser.act"

		ZI251 = lex_state->lx.start;
		ZI252   = lex_state->lx.end;
	
#line 2497 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF EXTRACT: OPENCOUNT */
					ADVANCE_LEXER;
					switch (CURRENT_TERMINAL) {
					case (TOK_COUNT):
						/* BEGINNING OF EXTRACT: COUNT */
						{
#line 411 "src/libre/parser.act"

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
	
#line 2525 "src/libre/dialect/pcre/parser.c"
						}
						/* END OF EXTRACT: COUNT */
						break;
					default:
						goto ZL4;
					}
					ADVANCE_LEXER;
					p_253 (flags, lex_state, act_state, err, &ZIe, &ZI251, &ZIm, &ZInode);
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
#line 616 "src/libre/parser.act"

		(ZIc) = ast_count(0, NULL, 1, NULL);
	
#line 2551 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: atom-opt */
					/* BEGINNING OF ACTION: ast-expr-atom */
					{
#line 596 "src/libre/parser.act"

		(ZInode) = re_ast_expr_with_count((ZIe), (ZIc));
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL4;
		}
	
#line 2564 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-expr-atom */
				}
				break;
			case (TOK_PLUS):
				{
					t_ast__count ZIc;

					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: atom-plus */
					{
#line 608 "src/libre/parser.act"

		(ZIc) = ast_count(1, NULL, AST_COUNT_UNBOUNDED, NULL);
	
#line 2580 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: atom-plus */
					/* BEGINNING OF ACTION: ast-expr-atom */
					{
#line 596 "src/libre/parser.act"

		(ZInode) = re_ast_expr_with_count((ZIe), (ZIc));
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL4;
		}
	
#line 2593 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-expr-atom */
				}
				break;
			case (TOK_STAR):
				{
					t_ast__count ZIc;

					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: atom-kleene */
					{
#line 604 "src/libre/parser.act"

		(ZIc) = ast_count(0, NULL, AST_COUNT_UNBOUNDED, NULL);
	
#line 2609 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: atom-kleene */
					/* BEGINNING OF ACTION: ast-expr-atom */
					{
#line 596 "src/libre/parser.act"

		(ZInode) = re_ast_expr_with_count((ZIe), (ZIc));
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL4;
		}
	
#line 2622 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-expr-atom */
				}
				break;
			default:
				{
					t_ast__count ZIc;

					/* BEGINNING OF ACTION: atom-one */
					{
#line 612 "src/libre/parser.act"

		(ZIc) = ast_count(1, NULL, 1, NULL);
	
#line 2637 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: atom-one */
					/* BEGINNING OF ACTION: ast-expr-atom */
					{
#line 596 "src/libre/parser.act"

		(ZInode) = re_ast_expr_with_count((ZIe), (ZIc));
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL4;
		}
	
#line 2650 "src/libre/dialect/pcre/parser.c"
					}
					/* END OF ACTION: ast-expr-atom */
				}
				break;
			}
			goto ZL3;
		ZL4:;
			{
				/* BEGINNING OF ACTION: err-expected-count */
				{
#line 472 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCOUNT;
		}
		goto ZL1;
	
#line 2668 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF ACTION: err-expected-count */
				ZInode = ZIe;
			}
		ZL3:;
		}
		/* END OF INLINE: 198 */
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
		p_expr_C_Clist_Hof_Hatoms (flags, lex_state, act_state, err, &ZInode);
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
p_250(flags flags, lex_state lex_state, act_state act_state, err err, t_char *ZI247, t_pos *ZI248, t_char__class__ast *ZOnode)
{
	t_char__class__ast ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_range__endpoint ZIlower;
			t_range__endpoint ZIupper;
			t_pos ZIu__end;
			t_pos ZI153;
			t_pos ZI154;

			/* BEGINNING OF ACTION: ast-range-endpoint-literal */
			{
#line 735 "src/libre/parser.act"

		struct ast_range_endpoint range;
		range.t = AST_RANGE_ENDPOINT_LITERAL;
		range.u.literal.c = (*ZI247);
		(ZIlower) = range;
	
#line 2731 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
			p_141 (flags, lex_state, act_state, err);
			p_145 (flags, lex_state, act_state, err, &ZIupper, &ZIu__end);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: mark-range */
			{
#line 547 "src/libre/parser.act"

		mark(&act_state->rangestart, &(*ZI248));
		mark(&act_state->rangeend,   &(ZIu__end));
	
#line 2747 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-range */
			ZI153 = *ZI248;
			ZI154 = ZIu__end;
			/* BEGINNING OF ACTION: char-class-ast-range */
			{
#line 660 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		unsigned char lower, upper;
		AST_POS_OF_LX_POS(ast_start, (*ZI248));
		AST_POS_OF_LX_POS(ast_end, (ZIu__end));

		if ((ZIlower).t != AST_RANGE_ENDPOINT_LITERAL ||
		    (ZIupper).t != AST_RANGE_ENDPOINT_LITERAL) {
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

		(ZInode) = re_char_class_ast_range(&(ZIlower), ast_start, &(ZIupper), ast_end);
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 2784 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: char-class-ast-range */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: char-class-ast-literal */
			{
#line 636 "src/libre/parser.act"

		(ZInode) = re_char_class_ast_literal((*ZI247));
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 2798 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: char-class-ast-literal */
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
p_253(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZIe, t_pos *ZI251, t_unsigned *ZIm, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_CLOSECOUNT):
		{
			t_pos ZI201;
			t_pos ZIend;
			t_ast__count ZIc;

			/* BEGINNING OF EXTRACT: CLOSECOUNT */
			{
#line 268 "src/libre/parser.act"

		ZI201 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 2833 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF EXTRACT: CLOSECOUNT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: mark-count */
			{
#line 552 "src/libre/parser.act"

		mark(&act_state->countstart, &(*ZI251));
		mark(&act_state->countend,   &(ZIend));
	
#line 2844 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-count */
			/* BEGINNING OF ACTION: expr-count */
			{
#line 620 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		if ((*ZIm) < (*ZIm)) {
			err->e = RE_ENEGCOUNT;
			err->m = (*ZIm);
			err->n = (*ZIm);
			mark(&act_state->countstart, &(*ZI251));
			mark(&act_state->countend,   &(ZIend));
			goto ZL1;
		}
		AST_POS_OF_LX_POS(ast_start, (*ZI251));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		(ZIc) = ast_count((*ZIm), &ast_start, (*ZIm), &ast_end);
	
#line 2865 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: expr-count */
			/* BEGINNING OF ACTION: ast-expr-atom */
			{
#line 596 "src/libre/parser.act"

		(ZInode) = re_ast_expr_with_count((*ZIe), (ZIc));
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL1;
		}
	
#line 2878 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-expr-atom */
		}
		break;
	case (TOK_SEP):
		{
			t_unsigned ZIn;
			t_pos ZIend;
			t_pos ZI204;
			t_ast__count ZIc;

			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (TOK_COUNT):
				/* BEGINNING OF EXTRACT: COUNT */
				{
#line 411 "src/libre/parser.act"

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
	
#line 2915 "src/libre/dialect/pcre/parser.c"
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
#line 268 "src/libre/parser.act"

		ZIend = lex_state->lx.start;
		ZI204   = lex_state->lx.end;
	
#line 2932 "src/libre/dialect/pcre/parser.c"
				}
				/* END OF EXTRACT: CLOSECOUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: mark-count */
			{
#line 552 "src/libre/parser.act"

		mark(&act_state->countstart, &(*ZI251));
		mark(&act_state->countend,   &(ZIend));
	
#line 2947 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: mark-count */
			/* BEGINNING OF ACTION: expr-count */
			{
#line 620 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		if ((ZIn) < (*ZIm)) {
			err->e = RE_ENEGCOUNT;
			err->m = (*ZIm);
			err->n = (ZIn);
			mark(&act_state->countstart, &(*ZI251));
			mark(&act_state->countend,   &(ZIend));
			goto ZL1;
		}
		AST_POS_OF_LX_POS(ast_start, (*ZI251));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		(ZIc) = ast_count((*ZIm), &ast_start, (ZIn), &ast_end);
	
#line 2968 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: expr-count */
			/* BEGINNING OF ACTION: ast-expr-atom */
			{
#line 596 "src/libre/parser.act"

		(ZInode) = re_ast_expr_with_count((*ZIe), (ZIc));
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL1;
		}
	
#line 2981 "src/libre/dialect/pcre/parser.c"
			}
			/* END OF ACTION: ast-expr-atom */
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

/* BEGINNING OF TRAILER */

#line 772 "src/libre/parser.act"


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

	struct ast_re *
	DIALECT_PARSE(re_getchar_fun *f, void *opaque,
		const struct fsm_options *opt,
		enum re_flags flags, int overlap,
		struct re_err *err)
	{
		struct ast_re *ast;
		struct flags top, *fl = &top;

		struct act_state  act_state_s;
		struct act_state *act_state;
		struct lex_state  lex_state_s;
		struct lex_state *lex_state;
		struct re_err dummy;

		struct LX_STATE *lx;

		top.flags = flags;

		assert(f != NULL);

		ast = re_ast_new();

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
			if (err->e == RE_ESUCCESS) { assert(0); }
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

		re_ast_free(ast);

		return NULL;
	}

#line 3145 "src/libre/dialect/pcre/parser.c"

/* END OF FILE */
