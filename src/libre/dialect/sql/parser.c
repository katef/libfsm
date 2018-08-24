/*
 * Automatically generated from the files:
 *	src/libre/dialect/sql/parser.sid
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

#line 219 "src/libre/dialect/sql/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

extern void p_re__sql(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Cchar_Hclass(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Cliteral(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Catom_Hsuffix(flags, lex_state, act_state, err, t_ast__count *);
static void p_189(flags, lex_state, act_state, err, t_ast__expr *, t_ast__expr *);
static void p_190(flags, lex_state, act_state, err, t_ast__expr *, t_ast__expr *);
static void p_191(flags, lex_state, act_state, err, t_pos *, t_unsigned *, t_ast__count *);
static void p_expr_C_Cchar_Hclass_C_Cchar_Hclass_Hhead(flags, lex_state, act_state, err, t_char__class__ast *);
static void p_expr(flags, lex_state, act_state, err, t_ast__expr *);
static void p_194(flags, lex_state, act_state, err, t_pos *, t_char__class__ast *, t_char__class__ast *, t_char__class__ast *, t_ast__expr *);
static void p_196(flags, lex_state, act_state, err, t_char__class__ast *);
static void p_200(flags, lex_state, act_state, err, t_char *, t_pos *, t_char__class__ast *);
static void p_201(flags, lex_state, act_state, err, t_ast__expr *, t_ast__expr *);
static void p_expr_C_Cchar_Hclass_C_Clist_Hof_Hchar_Hclass_Hterms(flags, lex_state, act_state, err, t_char__class__ast *);
static void p_expr_C_Cchar_Hclass_C_Cterm(flags, lex_state, act_state, err, t_char__class__ast *);
static void p_expr_C_Clist_Hof_Hatoms(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Cchar_Hclass_C_Cchar_Hclass_Htail(flags, lex_state, act_state, err, t_char__class__ast *);
static void p_expr_C_Clist_Hof_Halts(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Catom(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Cchar_Hclass_C_Cchar_Hclass_Hast(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Calt(flags, lex_state, act_state, err, t_ast__expr *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

void
p_re__sql(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 177 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY): case (TOK_MANY): case (TOK_OPENSUB): case (TOK_OPENGROUP):
			case (TOK_CHAR):
				{
					p_expr (flags, lex_state, act_state, err, &ZInode);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
				}
				break;
			default:
				{
					/* BEGINNING OF ACTION: ast-expr-empty */
					{
#line 559 "src/libre/parser.act"

		(ZInode) = re_ast_expr_empty();
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 286 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: ast-expr-empty */
				}
				break;
			}
		}
		/* END OF INLINE: 177 */
		/* BEGINNING OF INLINE: 178 */
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
	
#line 317 "src/libre/dialect/sql/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL3:;
		}
		/* END OF INLINE: 178 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Cchar_Hclass(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		p_expr_C_Cchar_Hclass_C_Cchar_Hclass_Hast (flags, lex_state, act_state, err, &ZInode);
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
p_expr_C_Cliteral(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

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
#line 401 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI94 = lex_state->lx.start;
		ZI95   = lex_state->lx.end;

		ZIc = lex_state->buf.a[0];
	
#line 383 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: ast-expr-literal */
		{
#line 574 "src/libre/parser.act"

		(ZInode) = re_ast_expr_literal((ZIc));
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 398 "src/libre/dialect/sql/parser.c"
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
p_expr_C_Catom_Hsuffix(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__count *ZOf)
{
	t_ast__count ZIf;

	switch (CURRENT_TERMINAL) {
	case (TOK_OPENCOUNT):
		{
			t_pos ZIpos__of;
			t_pos ZIpos__ot;
			t_unsigned ZIm;

			/* BEGINNING OF EXTRACT: OPENCOUNT */
			{
#line 263 "src/libre/parser.act"

		ZIpos__of = lex_state->lx.start;
		ZIpos__ot   = lex_state->lx.end;
	
#line 429 "src/libre/dialect/sql/parser.c"
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
			goto ZL1;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXCOUNT;
			goto ZL1;
		}

		ZIm = (unsigned int) u;
	
#line 457 "src/libre/dialect/sql/parser.c"
				}
				/* END OF EXTRACT: COUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			p_191 (flags, lex_state, act_state, err, &ZIpos__of, &ZIm, &ZIf);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_OPT):
		{
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: atom-opt */
			{
#line 616 "src/libre/parser.act"

		(ZIf) = ast_count(0, NULL, 1, NULL);
	
#line 481 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: atom-opt */
		}
		break;
	case (TOK_PLUS):
		{
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: atom-plus */
			{
#line 608 "src/libre/parser.act"

		(ZIf) = ast_count(1, NULL, AST_COUNT_UNBOUNDED, NULL);
	
#line 495 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: atom-plus */
		}
		break;
	case (TOK_STAR):
		{
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: atom-kleene */
			{
#line 604 "src/libre/parser.act"

		(ZIf) = ast_count(0, NULL, AST_COUNT_UNBOUNDED, NULL);
	
#line 509 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: atom-kleene */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: atom-one */
			{
#line 612 "src/libre/parser.act"

		(ZIf) = ast_count(1, NULL, 1, NULL);
	
#line 522 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: atom-one */
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
p_189(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZI187, t_ast__expr *ZOnode)
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

		(ZInode) = re_ast_expr_alt((*ZI187), (ZIr));
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 561 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-expr-alt */
		}
		break;
	default:
		{
			ZInode = *ZI187;
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
p_190(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZIa, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_ANY): case (TOK_MANY): case (TOK_OPENSUB): case (TOK_OPENGROUP):
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
	
#line 605 "src/libre/dialect/sql/parser.c"
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
	
#line 621 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-expr-empty */
			/* BEGINNING OF ACTION: ast-expr-concat */
			{
#line 564 "src/libre/parser.act"

		(ZInode) = re_ast_expr_concat((*ZIa), (ZIr));
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 631 "src/libre/dialect/sql/parser.c"
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

static void
p_191(flags flags, lex_state lex_state, act_state act_state, err err, t_pos *ZIpos__of, t_unsigned *ZIm, t_ast__count *ZOf)
{
	t_ast__count ZIf;

	switch (CURRENT_TERMINAL) {
	case (TOK_CLOSECOUNT):
		{
			t_pos ZIpos__cf;
			t_pos ZIpos__ct;

			/* BEGINNING OF EXTRACT: CLOSECOUNT */
			{
#line 268 "src/libre/parser.act"

		ZIpos__cf = lex_state->lx.start;
		ZIpos__ct   = lex_state->lx.end;
	
#line 665 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: CLOSECOUNT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: expr-count */
			{
#line 620 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		if ((*ZIm) < (*ZIm)) {
			err->e = RE_ENEGCOUNT;
			err->m = (*ZIm);
			err->n = (*ZIm);
			mark(&act_state->countstart, &(*ZIpos__of));
			mark(&act_state->countend,   &(ZIpos__ct));
			goto ZL1;
		}
		AST_POS_OF_LX_POS(ast_start, (*ZIpos__of));
		AST_POS_OF_LX_POS(ast_end, (ZIpos__ct));

		(ZIf) = ast_count((*ZIm), &ast_start, (*ZIm), &ast_end);
	
#line 687 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: expr-count */
		}
		break;
	case (TOK_SEP):
		{
			t_unsigned ZIn;
			t_pos ZIpos__cf;
			t_pos ZIpos__ct;

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
	
#line 723 "src/libre/dialect/sql/parser.c"
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

		ZIpos__cf = lex_state->lx.start;
		ZIpos__ct   = lex_state->lx.end;
	
#line 740 "src/libre/dialect/sql/parser.c"
				}
				/* END OF EXTRACT: CLOSECOUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: expr-count */
			{
#line 620 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		if ((ZIn) < (*ZIm)) {
			err->e = RE_ENEGCOUNT;
			err->m = (*ZIm);
			err->n = (ZIn);
			mark(&act_state->countstart, &(*ZIpos__of));
			mark(&act_state->countend,   &(ZIpos__ct));
			goto ZL1;
		}
		AST_POS_OF_LX_POS(ast_start, (*ZIpos__of));
		AST_POS_OF_LX_POS(ast_end, (ZIpos__ct));

		(ZIf) = ast_count((*ZIm), &ast_start, (ZIn), &ast_end);
	
#line 766 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: expr-count */
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
	*ZOf = ZIf;
}

static void
p_expr_C_Cchar_Hclass_C_Cchar_Hclass_Hhead(flags flags, lex_state lex_state, act_state act_state, err err, t_char__class__ast *ZOf)
{
	t_char__class__ast ZIf;

	switch (CURRENT_TERMINAL) {
	case (TOK_INVERT):
		{
			t_char ZI195;

			/* BEGINNING OF EXTRACT: INVERT */
			{
#line 242 "src/libre/parser.act"

		ZI195 = '^';
	
#line 800 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: INVERT */
			ADVANCE_LEXER;
			p_196 (flags, lex_state, act_state, err, &ZIf);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_RANGE):
		{
			t_char ZI110;
			t_pos ZI111;
			t_pos ZI112;

			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 246 "src/libre/parser.act"

		ZI110 = '-';
		ZI111 = lex_state->lx.start;
		ZI112   = lex_state->lx.end;
	
#line 825 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: char-class-ast-flag-minus */
			{
#line 725 "src/libre/parser.act"

		(ZIf) = re_char_class_ast_flags(RE_CHAR_CLASS_FLAG_MINUS);
		if ((ZIf) == NULL) { goto ZL1; }
	
#line 836 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: char-class-ast-flag-minus */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: char-class-ast-flag-none */
			{
#line 715 "src/libre/parser.act"

		(ZIf) = re_char_class_ast_flags(RE_CHAR_CLASS_FLAG_NONE);
		if ((ZIf) == NULL) { goto ZL1; }
	
#line 850 "src/libre/dialect/sql/parser.c"
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
		t_ast__expr ZI187;

		p_expr_C_Calt (flags, lex_state, act_state, err, &ZI187);
		p_189 (flags, lex_state, act_state, err, &ZI187, &ZInode);
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
p_194(flags flags, lex_state lex_state, act_state act_state, err err, t_pos *ZI192, t_char__class__ast *ZIhead, t_char__class__ast *ZIbody, t_char__class__ast *ZItail, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_INVERT):
		{
			t_char ZI156;
			t_char__class__ast ZImaskhead;
			t_char__class__ast ZImaskbody;
			t_char__class__ast ZImasktail;
			t_char__class__ast ZIhb;
			t_char__class__ast ZIhbt;
			t_char ZI160;
			t_pos ZI161;
			t_pos ZIend;
			t_char__class__ast ZImhb;
			t_char__class__ast ZImhbt;
			t_char__class__ast ZImasked;

			/* BEGINNING OF EXTRACT: INVERT */
			{
#line 242 "src/libre/parser.act"

		ZI156 = '^';
	
#line 919 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: INVERT */
			ADVANCE_LEXER;
			p_expr_C_Cchar_Hclass_C_Cchar_Hclass_Hhead (flags, lex_state, act_state, err, &ZImaskhead);
			p_expr_C_Cchar_Hclass_C_Clist_Hof_Hchar_Hclass_Hterms (flags, lex_state, act_state, err, &ZImaskbody);
			p_expr_C_Cchar_Hclass_C_Cchar_Hclass_Htail (flags, lex_state, act_state, err, &ZImasktail);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: char-class-ast-concat */
			{
#line 690 "src/libre/parser.act"

		(ZIhb) = re_char_class_ast_concat((*ZIhead), (*ZIbody));
		if ((ZIhb) == NULL) { goto ZL1; }
	
#line 937 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: char-class-ast-concat */
			/* BEGINNING OF ACTION: char-class-ast-concat */
			{
#line 690 "src/libre/parser.act"

		(ZIhbt) = re_char_class_ast_concat((ZIhb), (*ZItail));
		if ((ZIhbt) == NULL) { goto ZL1; }
	
#line 947 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: char-class-ast-concat */
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSEGROUP):
				/* BEGINNING OF EXTRACT: CLOSEGROUP */
				{
#line 257 "src/libre/parser.act"

		ZI160 = ']';
		ZI161 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 960 "src/libre/dialect/sql/parser.c"
				}
				/* END OF EXTRACT: CLOSEGROUP */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: err-unsupported */
			{
#line 535 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXUNSUPPORTD;
		}
		goto ZL1;
	
#line 977 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: err-unsupported */
			/* BEGINNING OF ACTION: char-class-ast-concat */
			{
#line 690 "src/libre/parser.act"

		(ZImhb) = re_char_class_ast_concat((ZImaskhead), (ZImaskbody));
		if ((ZImhb) == NULL) { goto ZL1; }
	
#line 987 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: char-class-ast-concat */
			/* BEGINNING OF ACTION: char-class-ast-concat */
			{
#line 690 "src/libre/parser.act"

		(ZImhbt) = re_char_class_ast_concat((ZImhb), (ZImasktail));
		if ((ZImhbt) == NULL) { goto ZL1; }
	
#line 997 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: char-class-ast-concat */
			/* BEGINNING OF ACTION: char-class-ast-subtract */
			{
#line 695 "src/libre/parser.act"

		(ZImasked) = re_char_class_ast_subtract((ZIhbt), (ZImhbt));
		if ((ZImasked) == NULL) { goto ZL1; }
	
#line 1007 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: char-class-ast-subtract */
			/* BEGINNING OF ACTION: ast-expr-char-class */
			{
#line 705 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		AST_POS_OF_LX_POS(ast_start, (*ZI192));
		AST_POS_OF_LX_POS(ast_end, (ZIend));
		mark(&act_state->groupstart, &(*ZI192));
		mark(&act_state->groupend,   &(ZIend));
		(ZInode) = re_ast_expr_char_class((ZImasked), &ast_start, &ast_end);
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 1022 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-expr-char-class */
		}
		break;
	case (TOK_CLOSEGROUP):
		{
			t_char__class__ast ZIhb;
			t_char__class__ast ZIhbt;
			t_char ZI153;
			t_pos ZI154;
			t_pos ZIend;

			/* BEGINNING OF ACTION: char-class-ast-concat */
			{
#line 690 "src/libre/parser.act"

		(ZIhb) = re_char_class_ast_concat((*ZIhead), (*ZIbody));
		if ((ZIhb) == NULL) { goto ZL1; }
	
#line 1042 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: char-class-ast-concat */
			/* BEGINNING OF ACTION: char-class-ast-concat */
			{
#line 690 "src/libre/parser.act"

		(ZIhbt) = re_char_class_ast_concat((ZIhb), (*ZItail));
		if ((ZIhbt) == NULL) { goto ZL1; }
	
#line 1052 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: char-class-ast-concat */
			/* BEGINNING OF EXTRACT: CLOSEGROUP */
			{
#line 257 "src/libre/parser.act"

		ZI153 = ']';
		ZI154 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 1063 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: CLOSEGROUP */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-expr-char-class */
			{
#line 705 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		AST_POS_OF_LX_POS(ast_start, (*ZI192));
		AST_POS_OF_LX_POS(ast_end, (ZIend));
		mark(&act_state->groupstart, &(*ZI192));
		mark(&act_state->groupend,   &(ZIend));
		(ZInode) = re_ast_expr_char_class((ZIhbt), &ast_start, &ast_end);
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 1079 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-expr-char-class */
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
p_196(flags flags, lex_state lex_state, act_state act_state, err err, t_char__class__ast *ZOf)
{
	t_char__class__ast ZIf;

	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZI114;
			t_pos ZI115;
			t_pos ZI116;

			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 246 "src/libre/parser.act"

		ZI114 = '-';
		ZI115 = lex_state->lx.start;
		ZI116   = lex_state->lx.end;
	
#line 1117 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: char-class-ast-flag-invert-minus */
			{
#line 730 "src/libre/parser.act"

		(ZIf) = re_char_class_ast_flags(RE_CHAR_CLASS_FLAG_INVERTED | RE_CHAR_CLASS_FLAG_MINUS);
		if ((ZIf) == NULL) { goto ZL1; }
	
#line 1128 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: char-class-ast-flag-invert-minus */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: char-class-ast-flag-invert */
			{
#line 720 "src/libre/parser.act"

		(ZIf) = re_char_class_ast_flags(RE_CHAR_CLASS_FLAG_INVERTED);
		if ((ZIf) == NULL) { goto ZL1; }
	
#line 1142 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: char-class-ast-flag-invert */
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
p_200(flags flags, lex_state lex_state, act_state act_state, err err, t_char *ZI197, t_pos *ZI198, t_char__class__ast *ZOnode)
{
	t_char__class__ast ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_range__endpoint ZIa;
			t_char ZI130;
			t_pos ZI131;
			t_pos ZI132;
			t_char ZIcz;
			t_pos ZI134;
			t_pos ZIend;
			t_range__endpoint ZIz;

			/* BEGINNING OF ACTION: ast-range-endpoint-literal */
			{
#line 735 "src/libre/parser.act"

		struct ast_range_endpoint range;
		range.t = AST_RANGE_ENDPOINT_LITERAL;
		range.u.literal.c = (*ZI197);
		(ZIa) = range;
	
#line 1184 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 246 "src/libre/parser.act"

		ZI130 = '-';
		ZI131 = lex_state->lx.start;
		ZI132   = lex_state->lx.end;
	
#line 1195 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				/* BEGINNING OF EXTRACT: CHAR */
				{
#line 401 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI134 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		ZIcz = lex_state->buf.a[0];
	
#line 1213 "src/libre/dialect/sql/parser.c"
				}
				/* END OF EXTRACT: CHAR */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-range-endpoint-literal */
			{
#line 735 "src/libre/parser.act"

		struct ast_range_endpoint range;
		range.t = AST_RANGE_ENDPOINT_LITERAL;
		range.u.literal.c = (ZIcz);
		(ZIz) = range;
	
#line 1230 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
			/* BEGINNING OF ACTION: char-class-ast-range */
			{
#line 660 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		unsigned char lower, upper;
		AST_POS_OF_LX_POS(ast_start, (*ZI198));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		if ((ZIa).t != AST_RANGE_ENDPOINT_LITERAL ||
		    (ZIz).t != AST_RANGE_ENDPOINT_LITERAL) {
			err->e = RE_EXUNSUPPORTD;
			goto ZL1;
		}

		lower = (ZIa).u.literal.c;
		upper = (ZIz).u.literal.c;

		if (lower > upper) {
			char a[5], b[5];
			
			assert(sizeof err->set >= 1 + sizeof a + 1 + sizeof b + 1 + 1);
			
			sprintf(err->set, "%s-%s",
			    escchar(a, sizeof a, lower), escchar(b, sizeof b, upper));
			err->e = RE_ENEGRANGE;
			goto ZL1;
		}

		(ZInode) = re_char_class_ast_range(&(ZIa), ast_start, &(ZIz), ast_end);
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 1265 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: char-class-ast-range */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: char-class-ast-literal */
			{
#line 636 "src/libre/parser.act"

		(ZInode) = re_char_class_ast_literal((*ZI197));
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 1279 "src/libre/dialect/sql/parser.c"
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
p_201(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZIa, t_ast__expr *ZOnode)
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
	
#line 1318 "src/libre/dialect/sql/parser.c"
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
	
#line 1334 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-expr-empty */
			/* BEGINNING OF ACTION: ast-expr-alt */
			{
#line 569 "src/libre/parser.act"

		(ZInode) = re_ast_expr_alt((*ZIa), (ZIr));
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 1344 "src/libre/dialect/sql/parser.c"
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
p_expr_C_Cchar_Hclass_C_Clist_Hof_Hchar_Hclass_Hterms(flags flags, lex_state lex_state, act_state act_state, err err, t_char__class__ast *ZOnode)
{
	t_char__class__ast ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char__class__ast ZIl;

		p_expr_C_Cchar_Hclass_C_Cterm (flags, lex_state, act_state, err, &ZIl);
		/* BEGINNING OF INLINE: 142 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_NAMED__CHAR__CLASS): case (TOK_CHAR):
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
	
#line 1391 "src/libre/dialect/sql/parser.c"
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
		/* END OF INLINE: 142 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Cchar_Hclass_C_Cterm(flags flags, lex_state lex_state, act_state act_state, err err, t_char__class__ast *ZOnode)
{
	t_char__class__ast ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_CHAR):
		{
			t_char ZI197;
			t_pos ZI198;
			t_pos ZI199;

			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 401 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI198 = lex_state->lx.start;
		ZI199   = lex_state->lx.end;

		ZI197 = lex_state->buf.a[0];
	
#line 1440 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			ADVANCE_LEXER;
			p_200 (flags, lex_state, act_state, err, &ZI197, &ZI198, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_NAMED__CHAR__CLASS):
		{
			t_ast__char__class__id ZIid;

			/* BEGINNING OF INLINE: expr::char-class::named-class */
			{
				{
					t_pos ZI122;
					t_pos ZI123;

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

		ZI122 = lex_state->lx.start;
		ZI123   = lex_state->lx.end;
	
#line 1486 "src/libre/dialect/sql/parser.c"
						}
						/* END OF EXTRACT: NAMED_CHAR_CLASS */
						break;
					default:
						goto ZL1;
					}
					ADVANCE_LEXER;
				}
			}
			/* END OF INLINE: expr::char-class::named-class */
			/* BEGINNING OF ACTION: char-class-ast-named-class */
			{
#line 700 "src/libre/parser.act"

		(ZInode) = re_char_class_ast_named_class((ZIid));
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 1504 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: char-class-ast-named-class */
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
	
#line 1526 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: err-expected-term */
		/* BEGINNING OF ACTION: char-class-ast-flag-none */
		{
#line 715 "src/libre/parser.act"

		(ZInode) = re_char_class_ast_flags(RE_CHAR_CLASS_FLAG_NONE);
		if ((ZInode) == NULL) { goto ZL3; }
	
#line 1536 "src/libre/dialect/sql/parser.c"
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
p_expr_C_Clist_Hof_Hatoms(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__expr ZIa;

		p_expr_C_Catom (flags, lex_state, act_state, err, &ZIa);
		p_190 (flags, lex_state, act_state, err, &ZIa, &ZInode);
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
p_expr_C_Cchar_Hclass_C_Cchar_Hclass_Htail(flags flags, lex_state lex_state, act_state act_state, err err, t_char__class__ast *ZOf)
{
	t_char__class__ast ZIf;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF ACTION: char-class-ast-flag-none */
		{
#line 715 "src/libre/parser.act"

		(ZIf) = re_char_class_ast_flags(RE_CHAR_CLASS_FLAG_NONE);
		if ((ZIf) == NULL) { goto ZL1; }
	
#line 1590 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: char-class-ast-flag-none */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOf = ZIf;
}

static void
p_expr_C_Clist_Hof_Halts(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__expr ZIa;

		p_expr_C_Calt (flags, lex_state, act_state, err, &ZIa);
		p_201 (flags, lex_state, act_state, err, &ZIa, &ZInode);
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
	
#line 1632 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: err-expected-alts */
		/* BEGINNING OF ACTION: ast-expr-empty */
		{
#line 559 "src/libre/parser.act"

		(ZInode) = re_ast_expr_empty();
		if ((ZInode) == NULL) { goto ZL2; }
	
#line 1642 "src/libre/dialect/sql/parser.c"
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
p_expr_C_Catom(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_ANY):
		{
			t_ast__count ZIs;

			ADVANCE_LEXER;
			p_expr_C_Catom_Hsuffix (flags, lex_state, act_state, err, &ZIs);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: ast-expr-atom-any */
			{
#line 584 "src/libre/parser.act"

		struct ast_expr *e = re_ast_expr_any();
		if (e == NULL) { goto ZL1; }
		(ZInode) = re_ast_expr_with_count(e, (ZIs));
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 1679 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-expr-atom-any */
		}
		break;
	case (TOK_MANY):
		{
			t_ast__count ZIs;
			t_ast__count ZIf;
			t_ast__expr ZIe;

			ADVANCE_LEXER;
			p_expr_C_Catom_Hsuffix (flags, lex_state, act_state, err, &ZIs);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: atom-kleene */
			{
#line 604 "src/libre/parser.act"

		(ZIf) = ast_count(0, NULL, AST_COUNT_UNBOUNDED, NULL);
	
#line 1702 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: atom-kleene */
			/* BEGINNING OF ACTION: ast-expr-atom-any */
			{
#line 584 "src/libre/parser.act"

		struct ast_expr *e = re_ast_expr_any();
		if (e == NULL) { goto ZL1; }
		(ZIe) = re_ast_expr_with_count(e, (ZIf));
		if ((ZIe) == NULL) { goto ZL1; }
	
#line 1714 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-expr-atom-any */
			/* BEGINNING OF ACTION: ast-expr-atom */
			{
#line 596 "src/libre/parser.act"

		(ZInode) = re_ast_expr_with_count((ZIe), (ZIs));
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL1;
		}
	
#line 1727 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-expr-atom */
		}
		break;
	case (TOK_OPENSUB):
		{
			t_ast__expr ZIg;
			t_ast__expr ZIe;
			t_ast__count ZIs;

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
	
#line 1751 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-expr-group */
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSESUB):
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			p_expr_C_Catom_Hsuffix (flags, lex_state, act_state, err, &ZIs);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: ast-expr-atom */
			{
#line 596 "src/libre/parser.act"

		(ZInode) = re_ast_expr_with_count((ZIe), (ZIs));
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL1;
		}
	
#line 1776 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-expr-atom */
		}
		break;
	case (TOK_OPENGROUP):
		{
			t_ast__expr ZIe;
			t_ast__count ZIs;

			p_expr_C_Cchar_Hclass (flags, lex_state, act_state, err, &ZIe);
			p_expr_C_Catom_Hsuffix (flags, lex_state, act_state, err, &ZIs);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: ast-expr-atom */
			{
#line 596 "src/libre/parser.act"

		(ZInode) = re_ast_expr_with_count((ZIe), (ZIs));
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL1;
		}
	
#line 1802 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-expr-atom */
		}
		break;
	case (TOK_CHAR):
		{
			t_ast__expr ZIe;
			t_ast__count ZIs;

			p_expr_C_Cliteral (flags, lex_state, act_state, err, &ZIe);
			p_expr_C_Catom_Hsuffix (flags, lex_state, act_state, err, &ZIs);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: ast-expr-atom */
			{
#line 596 "src/libre/parser.act"

		(ZInode) = re_ast_expr_with_count((ZIe), (ZIs));
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL1;
		}
	
#line 1828 "src/libre/dialect/sql/parser.c"
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

static void
p_expr_C_Cchar_Hclass_C_Cchar_Hclass_Hast(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_pos ZI192;
		t_pos ZI193;
		t_char__class__ast ZIhead;
		t_char__class__ast ZIbody;
		t_char__class__ast ZItail;

		switch (CURRENT_TERMINAL) {
		case (TOK_OPENGROUP):
			/* BEGINNING OF EXTRACT: OPENGROUP */
			{
#line 252 "src/libre/parser.act"

		ZI192 = lex_state->lx.start;
		ZI193   = lex_state->lx.end;
	
#line 1870 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: OPENGROUP */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		p_expr_C_Cchar_Hclass_C_Cchar_Hclass_Hhead (flags, lex_state, act_state, err, &ZIhead);
		p_expr_C_Cchar_Hclass_C_Clist_Hof_Hchar_Hclass_Hterms (flags, lex_state, act_state, err, &ZIbody);
		p_expr_C_Cchar_Hclass_C_Cchar_Hclass_Htail (flags, lex_state, act_state, err, &ZItail);
		p_194 (flags, lex_state, act_state, err, &ZI192, &ZIhead, &ZIbody, &ZItail, &ZInode);
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

#line 2064 "src/libre/dialect/sql/parser.c"

/* END OF FILE */
