/*
 * Automatically generated from the files:
 *	src/libre/dialect/sql/parser.sid
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

#line 215 "src/libre/dialect/sql/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_expr_C_Ccharacter_Hclass_C_Cclass_Hhead(flags, lex_state, act_state, err, t_ast__class);
extern void p_re__sql(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms(flags, lex_state, act_state, err, t_ast__class);
static void p_expr_C_Ccharacter_Hclass_C_Cclass_Htail(flags, lex_state, act_state, err, t_ast__class);
static void p_expr_C_Cliteral(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Catom_Hsuffix(flags, lex_state, act_state, err, t_ast__count *);
static void p_expr_C_Ccharacter_Hclass(flags, lex_state, act_state, err, t_ast__expr *);
static void p_187(flags, lex_state, act_state, err, t_pos *, t_unsigned *, t_ast__count *);
static void p_190(flags, lex_state, act_state, err, t_pos *, t_ast__class *, t_ast__expr *);
static void p_192(flags, lex_state, act_state, err, t_ast__class *);
static void p_expr(flags, lex_state, act_state, err, t_ast__expr *);
static void p_196(flags, lex_state, act_state, err, t_char *, t_pos *, t_ast__class *);
static void p_expr_C_Clist_Hof_Hatoms(flags, lex_state, act_state, err, t_ast__expr);
static void p_expr_C_Clist_Hof_Halts(flags, lex_state, act_state, err, t_ast__expr);
static void p_expr_C_Ccharacter_Hclass_C_Cterm(flags, lex_state, act_state, err, t_ast__class *);
static void p_expr_C_Catom(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Ccharacter_Hclass_C_Cnamed_Hclass(flags, lex_state, act_state, err, t_ast__class *);
static void p_expr_C_Calt(flags, lex_state, act_state, err, t_ast__expr *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_expr_C_Ccharacter_Hclass_C_Cclass_Hhead(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__class ZIclass)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_INVERT):
		{
			t_char ZI191;

			/* BEGINNING OF EXTRACT: INVERT */
			{
#line 239 "src/libre/parser.act"

		ZI191 = '^';
	
#line 262 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: INVERT */
			ADVANCE_LEXER;
			p_192 (flags, lex_state, act_state, err, &ZIclass);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_RANGE):
		{
			t_char ZI113;
			t_pos ZI114;
			t_pos ZI115;
			t_ast__class ZIf;

			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 243 "src/libre/parser.act"

		ZI113 = '-';
		ZI114 = lex_state->lx.start;
		ZI115   = lex_state->lx.end;
	
#line 288 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-make-class-flag-minus */
			{
#line 816 "src/libre/parser.act"

		(ZIf) = ast_make_class_flags(AST_CLASS_FLAG_MINUS);
		if ((ZIf) == NULL) {
			goto ZL1;
		}
	
#line 301 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-class-flag-minus */
			/* BEGINNING OF ACTION: ast-add-class-concat */
			{
#line 830 "src/libre/parser.act"

		if (!ast_add_class_concat((ZIclass), (ZIf))) {
			goto ZL1;
		}
	
#line 312 "src/libre/dialect/sql/parser.c"
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
	
#line 330 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-class-flag-none */
			/* BEGINNING OF ACTION: ast-add-class-concat */
			{
#line 830 "src/libre/parser.act"

		if (!ast_add_class_concat((ZIclass), (ZIf))) {
			goto ZL1;
		}
	
#line 341 "src/libre/dialect/sql/parser.c"
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

void
p_re__sql(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 175 */
		{
			{
				p_expr (flags, lex_state, act_state, err, &ZInode);
				if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
					RESTORE_LEXER;
					goto ZL1;
				}
			}
		}
		/* END OF INLINE: 175 */
		/* BEGINNING OF INLINE: 176 */
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
	
#line 398 "src/libre/dialect/sql/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL3:;
		}
		/* END OF INLINE: 176 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__class ZIclass)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms:;
	{
		/* BEGINNING OF INLINE: 144 */
		{
			{
				t_ast__class ZInode;

				p_expr_C_Ccharacter_Hclass_C_Cterm (flags, lex_state, act_state, err, &ZInode);
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
	
#line 440 "src/libre/dialect/sql/parser.c"
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
	
#line 456 "src/libre/dialect/sql/parser.c"
				}
				/* END OF ACTION: err-expected-term */
			}
		ZL3:;
		}
		/* END OF INLINE: 144 */
		/* BEGINNING OF INLINE: 145 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_NAMED__CLASS): case (TOK_CHAR):
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
		/* END OF INLINE: 145 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_expr_C_Ccharacter_Hclass_C_Cclass_Htail(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__class ZIclass)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__class ZIf;

		/* BEGINNING OF ACTION: ast-make-class-flag-none */
		{
#line 802 "src/libre/parser.act"

		(ZIf) = ast_make_class_flags(AST_CLASS_FLAG_NONE);
		if ((ZIf) == NULL) {
			goto ZL1;
		}
	
#line 503 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: ast-make-class-flag-none */
		/* BEGINNING OF ACTION: ast-add-class-concat */
		{
#line 830 "src/libre/parser.act"

		if (!ast_add_class_concat((ZIclass), (ZIf))) {
			goto ZL1;
		}
	
#line 514 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: ast-add-class-concat */
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
		t_pos ZI96;
		t_pos ZI97;

		switch (CURRENT_TERMINAL) {
		case (TOK_CHAR):
			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 401 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI96 = lex_state->lx.start;
		ZI97   = lex_state->lx.end;

		ZIc = lex_state->buf.a[0];
	
#line 551 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: ast-make-expr-literal */
		{
#line 646 "src/libre/parser.act"

		(ZInode) = ast_make_expr_literal((ZIc));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 568 "src/libre/dialect/sql/parser.c"
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
#line 260 "src/libre/parser.act"

		ZIpos__of = lex_state->lx.start;
		ZIpos__ot   = lex_state->lx.end;
	
#line 599 "src/libre/dialect/sql/parser.c"
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
			goto ZL1;
		}

		if ((u == ULONG_MAX && errno != 0) || *e != '\0') {
			err->e = RE_EXCOUNT;
			goto ZL1;
		}

		ZIm = (unsigned int) u;
	
#line 627 "src/libre/dialect/sql/parser.c"
				}
				/* END OF EXTRACT: COUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			p_187 (flags, lex_state, act_state, err, &ZIpos__of, &ZIm, &ZIf);
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
#line 570 "src/libre/parser.act"

		(ZIf) = ast_make_count(0, NULL, 1, NULL);
	
#line 651 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: atom-opt */
		}
		break;
	case (TOK_PLUS):
		{
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: atom-plus */
			{
#line 562 "src/libre/parser.act"

		(ZIf) = ast_make_count(1, NULL, AST_COUNT_UNBOUNDED, NULL);
	
#line 665 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: atom-plus */
		}
		break;
	case (TOK_STAR):
		{
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: atom-kleene */
			{
#line 558 "src/libre/parser.act"

		(ZIf) = ast_make_count(0, NULL, AST_COUNT_UNBOUNDED, NULL);
	
#line 679 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: atom-kleene */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: atom-one */
			{
#line 566 "src/libre/parser.act"

		(ZIf) = ast_make_count(1, NULL, 1, NULL);
	
#line 692 "src/libre/dialect/sql/parser.c"
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
p_expr_C_Ccharacter_Hclass(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_pos ZI188;
		t_pos ZI189;
		t_ast__class ZIclass;

		switch (CURRENT_TERMINAL) {
		case (TOK_OPENGROUP):
			/* BEGINNING OF EXTRACT: OPENGROUP */
			{
#line 249 "src/libre/parser.act"

		ZI188 = lex_state->lx.start;
		ZI189   = lex_state->lx.end;
	
#line 730 "src/libre/dialect/sql/parser.c"
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
	
#line 747 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: ast-make-class-concat */
		p_expr_C_Ccharacter_Hclass_C_Cclass_Hhead (flags, lex_state, act_state, err, ZIclass);
		p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms (flags, lex_state, act_state, err, ZIclass);
		p_expr_C_Ccharacter_Hclass_C_Cclass_Htail (flags, lex_state, act_state, err, ZIclass);
		p_190 (flags, lex_state, act_state, err, &ZI188, &ZIclass, &ZInode);
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
p_187(flags flags, lex_state lex_state, act_state act_state, err err, t_pos *ZIpos__of, t_unsigned *ZIm, t_ast__count *ZOf)
{
	t_ast__count ZIf;

	switch (CURRENT_TERMINAL) {
	case (TOK_CLOSECOUNT):
		{
			t_pos ZIpos__cf;
			t_pos ZIpos__ct;

			/* BEGINNING OF EXTRACT: CLOSECOUNT */
			{
#line 265 "src/libre/parser.act"

		ZIpos__cf = lex_state->lx.start;
		ZIpos__ct   = lex_state->lx.end;
	
#line 785 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: CLOSECOUNT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: mark-count */
			{
#line 540 "src/libre/parser.act"

		mark(&act_state->countstart, &(*ZIpos__of));
		mark(&act_state->countend,   &(ZIpos__ct));
	
#line 796 "src/libre/dialect/sql/parser.c"
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

			mark(&act_state->countstart, &(*ZIpos__of));
			mark(&act_state->countend,   &(ZIpos__ct));

			goto ZL1;
		}

		AST_POS_OF_LX_POS(ast_start, (*ZIpos__of));
		AST_POS_OF_LX_POS(ast_end, (ZIpos__ct));

		(ZIf) = ast_make_count((*ZIm), &ast_start, (*ZIm), &ast_end);
	
#line 821 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: atom-count */
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
	
#line 857 "src/libre/dialect/sql/parser.c"
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

		ZIpos__cf = lex_state->lx.start;
		ZIpos__ct   = lex_state->lx.end;
	
#line 874 "src/libre/dialect/sql/parser.c"
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

		mark(&act_state->countstart, &(*ZIpos__of));
		mark(&act_state->countend,   &(ZIpos__ct));
	
#line 889 "src/libre/dialect/sql/parser.c"
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

			mark(&act_state->countstart, &(*ZIpos__of));
			mark(&act_state->countend,   &(ZIpos__ct));

			goto ZL1;
		}

		AST_POS_OF_LX_POS(ast_start, (*ZIpos__of));
		AST_POS_OF_LX_POS(ast_end, (ZIpos__ct));

		(ZIf) = ast_make_count((*ZIm), &ast_start, (ZIn), &ast_end);
	
#line 914 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: atom-count */
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
p_190(flags flags, lex_state lex_state, act_state act_state, err err, t_pos *ZI188, t_ast__class *ZIclass, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_INVERT):
		{
			t_char ZI152;
			t_ast__class ZImask;
			t_pos ZIend;
			t_ast__class ZIsub;

			/* BEGINNING OF EXTRACT: INVERT */
			{
#line 239 "src/libre/parser.act"

		ZI152 = '^';
	
#line 951 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: INVERT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-make-class-concat */
			{
#line 741 "src/libre/parser.act"

		(ZImask) = ast_make_class_concat();
		if ((ZImask) == NULL) {
			goto ZL1;
		}
	
#line 964 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-class-concat */
			p_expr_C_Ccharacter_Hclass_C_Cclass_Hhead (flags, lex_state, act_state, err, ZImask);
			p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms (flags, lex_state, act_state, err, ZImask);
			p_expr_C_Ccharacter_Hclass_C_Cclass_Htail (flags, lex_state, act_state, err, ZImask);
			/* BEGINNING OF INLINE: 154 */
			{
				if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
					RESTORE_LEXER;
					goto ZL1;
				}
				{
					t_char ZI155;
					t_pos ZI156;

					switch (CURRENT_TERMINAL) {
					case (TOK_CLOSEGROUP):
						/* BEGINNING OF EXTRACT: CLOSEGROUP */
						{
#line 254 "src/libre/parser.act"

		ZI155 = ']';
		ZI156 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 990 "src/libre/dialect/sql/parser.c"
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

		mark(&act_state->groupstart, &(*ZI188));
		mark(&act_state->groupend,   &(ZIend));
	
#line 1005 "src/libre/dialect/sql/parser.c"
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
	
#line 1021 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: err-expected-closegroup */
					ZIend = *ZI188;
				}
			ZL2:;
			}
			/* END OF INLINE: 154 */
			/* BEGINNING OF ACTION: err-unsupported */
			{
#line 526 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXUNSUPPORTD;
		}
		goto ZL1;
	
#line 1038 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: err-unsupported */
			/* BEGINNING OF ACTION: ast-make-class-subtract */
			{
#line 795 "src/libre/parser.act"

		(ZIsub) = ast_make_class_subtract((*ZIclass), (ZImask));
		if ((ZIsub) == NULL) {
			goto ZL1;
		}
	
#line 1050 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-class-subtract */
			/* BEGINNING OF ACTION: ast-make-expr-class */
			{
#line 684 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;

		AST_POS_OF_LX_POS(ast_start, (*ZI188));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		mark(&act_state->groupstart, &(*ZI188));
		mark(&act_state->groupend,   &(ZIend));

		(ZInode) = ast_make_expr_class((ZIsub), &ast_start, &ast_end);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1070 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-expr-class */
		}
		break;
	case (TOK_CLOSEGROUP):
		{
			t_pos ZIend;

			/* BEGINNING OF INLINE: 147 */
			{
				{
					t_char ZI148;
					t_pos ZI149;

					switch (CURRENT_TERMINAL) {
					case (TOK_CLOSEGROUP):
						/* BEGINNING OF EXTRACT: CLOSEGROUP */
						{
#line 254 "src/libre/parser.act"

		ZI148 = ']';
		ZI149 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 1095 "src/libre/dialect/sql/parser.c"
						}
						/* END OF EXTRACT: CLOSEGROUP */
						break;
					default:
						goto ZL5;
					}
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: mark-group */
					{
#line 530 "src/libre/parser.act"

		mark(&act_state->groupstart, &(*ZI188));
		mark(&act_state->groupend,   &(ZIend));
	
#line 1110 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: mark-group */
				}
				goto ZL4;
			ZL5:;
				{
					/* BEGINNING OF ACTION: err-expected-closegroup */
					{
#line 491 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCLOSEGROUP;
		}
		goto ZL1;
	
#line 1126 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: err-expected-closegroup */
					ZIend = *ZI188;
				}
			ZL4:;
			}
			/* END OF INLINE: 147 */
			/* BEGINNING OF ACTION: ast-make-expr-class */
			{
#line 684 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;

		AST_POS_OF_LX_POS(ast_start, (*ZI188));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		mark(&act_state->groupstart, &(*ZI188));
		mark(&act_state->groupend,   &(ZIend));

		(ZInode) = ast_make_expr_class((*ZIclass), &ast_start, &ast_end);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1151 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-expr-class */
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
p_192(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__class *ZIclass)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZI117;
			t_pos ZI118;
			t_pos ZI119;
			t_ast__class ZIf;

			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 243 "src/libre/parser.act"

		ZI117 = '-';
		ZI118 = lex_state->lx.start;
		ZI119   = lex_state->lx.end;
	
#line 1188 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-make-class-flag-invert-minus */
			{
#line 823 "src/libre/parser.act"

		(ZIf) = ast_make_class_flags(AST_CLASS_FLAG_INVERTED | AST_CLASS_FLAG_MINUS);
		if ((ZIf) == NULL) {
			goto ZL1;
		}
	
#line 1201 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-class-flag-invert-minus */
			/* BEGINNING OF ACTION: ast-add-class-concat */
			{
#line 830 "src/libre/parser.act"

		if (!ast_add_class_concat((*ZIclass), (ZIf))) {
			goto ZL1;
		}
	
#line 1212 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-add-class-concat */
		}
		break;
	default:
		{
			t_ast__class ZIf;

			/* BEGINNING OF ACTION: ast-make-class-flag-invert */
			{
#line 809 "src/libre/parser.act"

		(ZIf) = ast_make_class_flags(AST_CLASS_FLAG_INVERTED);
		if ((ZIf) == NULL) {
			goto ZL1;
		}
	
#line 1230 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-class-flag-invert */
			/* BEGINNING OF ACTION: ast-add-class-concat */
			{
#line 830 "src/libre/parser.act"

		if (!ast_add_class_concat((*ZIclass), (ZIf))) {
			goto ZL1;
		}
	
#line 1241 "src/libre/dialect/sql/parser.c"
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
	
#line 1273 "src/libre/dialect/sql/parser.c"
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
	
#line 1294 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: err-expected-alts */
		/* BEGINNING OF ACTION: ast-make-expr-empty */
		{
#line 625 "src/libre/parser.act"

		(ZInode) = ast_make_expr_empty();
		if ((ZInode) == NULL) {
			goto ZL2;
		}
	
#line 1306 "src/libre/dialect/sql/parser.c"
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
p_196(flags flags, lex_state lex_state, act_state act_state, err err, t_char *ZI193, t_pos *ZI194, t_ast__class *ZOnode)
{
	t_ast__class ZInode;

	switch (CURRENT_TERMINAL) {
	default:
		{
			/* BEGINNING OF ACTION: ast-make-class-literal */
			{
#line 748 "src/libre/parser.act"

		(ZInode) = ast_make_class_literal((*ZI193));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1335 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-class-literal */
		}
		break;
	case (TOK_RANGE):
		{
			t_endpoint ZIa;
			t_char ZI133;
			t_pos ZI134;
			t_pos ZI135;
			t_char ZIcz;
			t_pos ZI137;
			t_pos ZIend;
			t_endpoint ZIz;

			/* BEGINNING OF ACTION: ast-range-endpoint-literal */
			{
#line 594 "src/libre/parser.act"

		(ZIa).type = AST_ENDPOINT_LITERAL;
		(ZIa).u.literal.c = (*ZI193);
	
#line 1358 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 243 "src/libre/parser.act"

		ZI133 = '-';
		ZI134 = lex_state->lx.start;
		ZI135   = lex_state->lx.end;
	
#line 1369 "src/libre/dialect/sql/parser.c"
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

		ZI137 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		ZIcz = lex_state->buf.a[0];
	
#line 1387 "src/libre/dialect/sql/parser.c"
				}
				/* END OF EXTRACT: CHAR */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-range-endpoint-literal */
			{
#line 594 "src/libre/parser.act"

		(ZIz).type = AST_ENDPOINT_LITERAL;
		(ZIz).u.literal.c = (ZIcz);
	
#line 1402 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
			/* BEGINNING OF ACTION: mark-range */
			{
#line 535 "src/libre/parser.act"

		mark(&act_state->rangestart, &(*ZI194));
		mark(&act_state->rangeend,   &(ZIend));
	
#line 1412 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: mark-range */
			/* BEGINNING OF ACTION: ast-make-class-range */
			{
#line 758 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		unsigned char lower, upper;

		AST_POS_OF_LX_POS(ast_start, (*ZI194));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		if ((ZIa).type != AST_ENDPOINT_LITERAL ||
			(ZIz).type != AST_ENDPOINT_LITERAL) {
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

		(ZInode) = ast_make_class_range(&(ZIa), ast_start, &(ZIz), ast_end);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1450 "src/libre/dialect/sql/parser.c"
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
	
#line 1489 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: ast-add-expr-concat */
		/* BEGINNING OF INLINE: 167 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY): case (TOK_MANY): case (TOK_OPENSUB): case (TOK_OPENGROUP):
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
		/* END OF INLINE: 167 */
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
	
#line 1538 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: ast-add-expr-alt */
		/* BEGINNING OF INLINE: 173 */
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
		/* END OF INLINE: 173 */
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
	
#line 1570 "src/libre/dialect/sql/parser.c"
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
p_expr_C_Ccharacter_Hclass_C_Cterm(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__class *ZOnode)
{
	t_ast__class ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_CHAR):
		{
			t_char ZI193;
			t_pos ZI194;
			t_pos ZI195;

			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 401 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI194 = lex_state->lx.start;
		ZI195   = lex_state->lx.end;

		ZI193 = lex_state->buf.a[0];
	
#line 1605 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			ADVANCE_LEXER;
			p_196 (flags, lex_state, act_state, err, &ZI193, &ZI194, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_NAMED__CLASS):
		{
			p_expr_C_Ccharacter_Hclass_C_Cnamed_Hclass (flags, lex_state, act_state, err, &ZInode);
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
			/* BEGINNING OF ACTION: ast-make-expr-atom-any */
			{
#line 672 "src/libre/parser.act"

		struct ast_expr *e;

		e = ast_make_expr_any();
		if (e == NULL) {
			goto ZL1;
		}

		(ZInode) = ast_make_expr_with_count(e, (ZIs));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1670 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-expr-atom-any */
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
#line 558 "src/libre/parser.act"

		(ZIf) = ast_make_count(0, NULL, AST_COUNT_UNBOUNDED, NULL);
	
#line 1693 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: atom-kleene */
			/* BEGINNING OF ACTION: ast-make-expr-atom-any */
			{
#line 672 "src/libre/parser.act"

		struct ast_expr *e;

		e = ast_make_expr_any();
		if (e == NULL) {
			goto ZL1;
		}

		(ZIe) = ast_make_expr_with_count(e, (ZIf));
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 1712 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-expr-atom-any */
			/* BEGINNING OF ACTION: ast-make-expr-atom */
			{
#line 660 "src/libre/parser.act"

		(ZInode) = ast_make_expr_with_count((ZIe), (ZIs));
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL1;
		}
	
#line 1725 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-expr-atom */
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
			/* BEGINNING OF ACTION: ast-make-expr-group */
			{
#line 697 "src/libre/parser.act"

		(ZIe) = ast_make_expr_group((ZIg));
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 1751 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-expr-group */
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
			/* BEGINNING OF ACTION: ast-make-expr-atom */
			{
#line 660 "src/libre/parser.act"

		(ZInode) = ast_make_expr_with_count((ZIe), (ZIs));
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL1;
		}
	
#line 1776 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-expr-atom */
		}
		break;
	case (TOK_OPENGROUP):
		{
			t_ast__expr ZIe;
			t_ast__count ZIs;

			p_expr_C_Ccharacter_Hclass (flags, lex_state, act_state, err, &ZIe);
			p_expr_C_Catom_Hsuffix (flags, lex_state, act_state, err, &ZIs);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
			/* BEGINNING OF ACTION: ast-make-expr-atom */
			{
#line 660 "src/libre/parser.act"

		(ZInode) = ast_make_expr_with_count((ZIe), (ZIs));
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL1;
		}
	
#line 1802 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-expr-atom */
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
			/* BEGINNING OF ACTION: ast-make-expr-atom */
			{
#line 660 "src/libre/parser.act"

		(ZInode) = ast_make_expr_with_count((ZIe), (ZIs));
		if ((ZInode) == NULL) {
			err->e = RE_EXEOF;
			goto ZL1;
		}
	
#line 1828 "src/libre/dialect/sql/parser.c"
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
p_expr_C_Ccharacter_Hclass_C_Cnamed_Hclass(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__class *ZOnode)
{
	t_ast__class ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__class__id ZIid;
		t_pos ZI125;
		t_pos ZI126;

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

		ZI125 = lex_state->lx.start;
		ZI126   = lex_state->lx.end;
	
#line 1874 "src/libre/dialect/sql/parser.c"
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

		(ZInode) = ast_make_class_named((ZIid));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1891 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: ast-make-class-named */
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
	case (TOK_ANY): case (TOK_MANY): case (TOK_OPENSUB): case (TOK_OPENGROUP):
	case (TOK_CHAR):
		{
			/* BEGINNING OF ACTION: ast-make-expr-concat */
			{
#line 632 "src/libre/parser.act"

		(ZInode) = ast_make_expr_concat();
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1921 "src/libre/dialect/sql/parser.c"
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
	
#line 1942 "src/libre/dialect/sql/parser.c"
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

/* BEGINNING OF TRAILER */

#line 978 "src/libre/parser.act"


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

		ast_free(ast);

		return NULL;
	}

#line 2104 "src/libre/dialect/sql/parser.c"

/* END OF FILE */
