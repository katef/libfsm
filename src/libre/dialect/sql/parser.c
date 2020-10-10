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

#line 210 "src/libre/dialect/sql/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_expr_C_Ccharacter_Hclass_C_Cclass_Hhead(flags, lex_state, act_state, err, t_ast__expr *);
extern void p_re__sql(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms(flags, lex_state, act_state, err, t_ast__expr);
static void p_expr_C_Ccharacter_Hclass_C_Cclass_Htail(flags, lex_state, act_state, err, t_ast__expr);
static void p_expr_C_Clist_Hof_Hpieces(flags, lex_state, act_state, err, t_ast__expr);
static void p_expr_C_Ccharacter_Hclass_C_Cclass_Hterm(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Ccharacter_Hclass(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Cpiece(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr(flags, lex_state, act_state, err, t_ast__expr *);
static void p_194(flags, lex_state, act_state, err, t_ast__expr *);
static void p_198(flags, lex_state, act_state, err, t_char *, t_pos *, t_ast__expr *);
static void p_201(flags, lex_state, act_state, err, t_pos *, t_unsigned *, t_ast__count *);
static void p_expr_C_Clist_Hof_Halts(flags, lex_state, act_state, err, t_ast__expr);
static void p_expr_C_Cpiece_C_Ccount(flags, lex_state, act_state, err, t_ast__count *);
static void p_expr_C_Cpiece_C_Catom(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Ccharacter_Hclass_C_Cclass_Hnamed(flags, lex_state, act_state, err, t_ast__expr *);
static void p_expr_C_Calt(flags, lex_state, act_state, err, t_ast__expr *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_expr_C_Ccharacter_Hclass_C_Cclass_Hhead(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZIclass)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_INVERT):
		{
			t_char ZI193;

			/* BEGINNING OF EXTRACT: INVERT */
			{
#line 233 "src/libre/parser.act"

		ZI193 = '^';
	
#line 256 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: INVERT */
			ADVANCE_LEXER;
			p_194 (flags, lex_state, act_state, err, ZIclass);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_RANGE):
		{
			t_char ZIc;
			t_pos ZI101;
			t_pos ZI102;
			t_ast__expr ZInode;

			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 237 "src/libre/parser.act"

		ZIc = '-';
		ZI101 = lex_state->lx.start;
		ZI102   = lex_state->lx.end;
	
#line 282 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-make-literal */
			{
#line 713 "src/libre/parser.act"

		(ZInode) = ast_make_expr_literal(act_state->poolp, *flags, (ZIc));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 295 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-literal */
			/* BEGINNING OF ACTION: ast-add-alt */
			{
#line 867 "src/libre/parser.act"

		if (!ast_add_expr_alt((*ZIclass), (ZInode))) {
			goto ZL1;
		}
	
#line 306 "src/libre/dialect/sql/parser.c"
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

void
p_re__sql(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 180 */
		{
			{
				p_expr (flags, lex_state, act_state, err, &ZInode);
				if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
					RESTORE_LEXER;
					goto ZL1;
				}
			}
		}
		/* END OF INLINE: 180 */
		/* BEGINNING OF INLINE: 181 */
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
	
#line 365 "src/libre/dialect/sql/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL3:;
		}
		/* END OF INLINE: 181 */
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
		/* BEGINNING OF INLINE: 134 */
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
#line 867 "src/libre/parser.act"

		if (!ast_add_expr_alt((ZIclass), (ZInode))) {
			goto ZL4;
		}
	
#line 407 "src/libre/dialect/sql/parser.c"
				}
				/* END OF ACTION: ast-add-alt */
			}
			goto ZL3;
		ZL4:;
			{
				/* BEGINNING OF ACTION: err-expected-term */
				{
#line 513 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXTERM;
		}
		goto ZL1;
	
#line 423 "src/libre/dialect/sql/parser.c"
				}
				/* END OF ACTION: err-expected-term */
			}
		ZL3:;
		}
		/* END OF INLINE: 134 */
		/* BEGINNING OF INLINE: 135 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_NAMED__CLASS): case (TOK_CHAR):
				{
					/* BEGINNING OF INLINE: expr::character-class::list-of-class-terms */
					goto ZL2_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms;
					/* END OF INLINE: expr::character-class::list-of-class-terms */
				}
				/* UNREACHED */
			default:
				break;
			}
		}
		/* END OF INLINE: 135 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_expr_C_Ccharacter_Hclass_C_Cclass_Htail(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr ZIclass)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
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
#line 861 "src/libre/parser.act"

		if (!ast_add_expr_concat((ZIcat), (ZIa))) {
			goto ZL1;
		}
	
#line 483 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: ast-add-concat */
		/* BEGINNING OF INLINE: 172 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY): case (TOK_MANY): case (TOK_OPENSUB): case (TOK_OPENGROUP):
			case (TOK_CHAR):
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
		/* END OF INLINE: 172 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

static void
p_expr_C_Ccharacter_Hclass_C_Cclass_Hterm(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	case (TOK_CHAR):
		{
			t_char ZI195;
			t_pos ZI196;
			t_pos ZI197;

			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 451 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI196 = lex_state->lx.start;
		ZI197   = lex_state->lx.end;

		ZI195 = lex_state->buf.a[0];
	
#line 533 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			ADVANCE_LEXER;
			p_198 (flags, lex_state, act_state, err, &ZI195, &ZI196, &ZInode);
			if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
				RESTORE_LEXER;
				goto ZL1;
			}
		}
		break;
	case (TOK_NAMED__CLASS):
		{
			p_expr_C_Ccharacter_Hclass_C_Cclass_Hnamed (flags, lex_state, act_state, err, &ZInode);
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
		t_pos ZIopen__start;
		t_pos ZIopen__end;
		t_ast__expr ZIclass;
		t_ast__expr ZItmp;

		switch (CURRENT_TERMINAL) {
		case (TOK_OPENGROUP):
			/* BEGINNING OF EXTRACT: OPENGROUP */
			{
#line 243 "src/libre/parser.act"

		ZIopen__start = lex_state->lx.start;
		ZIopen__end   = lex_state->lx.end;
	
#line 589 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: OPENGROUP */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: ast-make-alt */
		{
#line 706 "src/libre/parser.act"

		(ZIclass) = ast_make_expr_alt(act_state->poolp, *flags);
		if ((ZIclass) == NULL) {
			goto ZL1;
		}
	
#line 606 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: ast-make-alt */
		ZItmp = ZIclass;
		p_expr_C_Ccharacter_Hclass_C_Cclass_Hhead (flags, lex_state, act_state, err, &ZIclass);
		p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms (flags, lex_state, act_state, err, ZItmp);
		p_expr_C_Ccharacter_Hclass_C_Cclass_Htail (flags, lex_state, act_state, err, ZItmp);
		/* BEGINNING OF INLINE: 140 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CLOSEGROUP):
				{
					t_char ZI141;
					t_pos ZIclose__start;
					t_pos ZIclose__end;

					/* BEGINNING OF EXTRACT: CLOSEGROUP */
					{
#line 263 "src/libre/parser.act"

		ZI141 = ']';
		ZIclose__start = lex_state->lx.start;
		ZIclose__end   = lex_state->lx.end;
	
#line 630 "src/libre/dialect/sql/parser.c"
					}
					/* END OF EXTRACT: CLOSEGROUP */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: mark-group */
					{
#line 590 "src/libre/parser.act"

		mark(&act_state->groupstart, &(ZIopen__start));
		mark(&act_state->groupend,   &(ZIopen__end));
	
#line 641 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: mark-group */
					/* BEGINNING OF ACTION: mark-expr */
					{
#line 605 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;

		AST_POS_OF_LX_POS(ast_start, (ZIopen__start));
		AST_POS_OF_LX_POS(ast_end, (ZIclose__end));

		mark(&act_state->groupstart, &(ZIopen__start));
		mark(&act_state->groupend,   &(ZIclose__end));

/* TODO: reinstate this, applies to an expr node in general
		(ZItmp)->u.class.start = ast_start;
		(ZItmp)->u.class.end   = ast_end;
*/
	
#line 661 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: mark-expr */
					ZInode = ZIclass;
				}
				break;
			case (TOK_INVERT):
				{
					t_char ZI145;
					t_ast__expr ZImask;
					t_ast__expr ZImask__tmp;
					t_pos ZIclose__end;

					/* BEGINNING OF EXTRACT: INVERT */
					{
#line 233 "src/libre/parser.act"

		ZI145 = '^';
	
#line 680 "src/libre/dialect/sql/parser.c"
					}
					/* END OF EXTRACT: INVERT */
					ADVANCE_LEXER;
					/* BEGINNING OF ACTION: ast-make-alt */
					{
#line 706 "src/libre/parser.act"

		(ZImask) = ast_make_expr_alt(act_state->poolp, *flags);
		if ((ZImask) == NULL) {
			goto ZL3;
		}
	
#line 693 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: ast-make-alt */
					ZImask__tmp = ZImask;
					p_expr_C_Ccharacter_Hclass_C_Cclass_Hhead (flags, lex_state, act_state, err, &ZImask);
					p_expr_C_Ccharacter_Hclass_C_Clist_Hof_Hclass_Hterms (flags, lex_state, act_state, err, ZImask__tmp);
					p_expr_C_Ccharacter_Hclass_C_Cclass_Htail (flags, lex_state, act_state, err, ZImask__tmp);
					/* BEGINNING OF INLINE: 149 */
					{
						if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
							RESTORE_LEXER;
							goto ZL3;
						}
						{
							t_char ZI150;
							t_pos ZIclose__start;

							switch (CURRENT_TERMINAL) {
							case (TOK_CLOSEGROUP):
								/* BEGINNING OF EXTRACT: CLOSEGROUP */
								{
#line 263 "src/libre/parser.act"

		ZI150 = ']';
		ZIclose__start = lex_state->lx.start;
		ZIclose__end   = lex_state->lx.end;
	
#line 720 "src/libre/dialect/sql/parser.c"
								}
								/* END OF EXTRACT: CLOSEGROUP */
								break;
							default:
								goto ZL5;
							}
							ADVANCE_LEXER;
							/* BEGINNING OF ACTION: mark-group */
							{
#line 590 "src/libre/parser.act"

		mark(&act_state->groupstart, &(ZIclose__start));
		mark(&act_state->groupend,   &(ZIclose__end));
	
#line 735 "src/libre/dialect/sql/parser.c"
							}
							/* END OF ACTION: mark-group */
						}
						goto ZL4;
					ZL5:;
						{
							t_pos ZIclose__start;

							/* BEGINNING OF ACTION: err-expected-closegroup */
							{
#line 548 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCLOSEGROUP;
		}
		goto ZL3;
	
#line 753 "src/libre/dialect/sql/parser.c"
							}
							/* END OF ACTION: err-expected-closegroup */
							ZIclose__start = ZIopen__end;
							ZIclose__end = ZIopen__end;
						}
					ZL4:;
					}
					/* END OF INLINE: 149 */
					/* BEGINNING OF ACTION: mark-expr */
					{
#line 605 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;

		AST_POS_OF_LX_POS(ast_start, (ZIopen__start));
		AST_POS_OF_LX_POS(ast_end, (ZIclose__end));

		mark(&act_state->groupstart, &(ZIopen__start));
		mark(&act_state->groupend,   &(ZIclose__end));

/* TODO: reinstate this, applies to an expr node in general
		(ZItmp)->u.class.start = ast_start;
		(ZItmp)->u.class.end   = ast_end;
*/
	
#line 779 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: mark-expr */
					/* BEGINNING OF ACTION: mark-expr */
					{
#line 605 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;

		AST_POS_OF_LX_POS(ast_start, (ZIopen__start));
		AST_POS_OF_LX_POS(ast_end, (ZIclose__end));

		mark(&act_state->groupstart, &(ZIopen__start));
		mark(&act_state->groupend,   &(ZIclose__end));

/* TODO: reinstate this, applies to an expr node in general
		(ZImask__tmp)->u.class.start = ast_start;
		(ZImask__tmp)->u.class.end   = ast_end;
*/
	
#line 799 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: mark-expr */
					/* BEGINNING OF ACTION: ast-make-subtract */
					{
#line 776 "src/libre/parser.act"

		(ZInode) = ast_make_expr_subtract(act_state->poolp, *flags, (ZIclass), (ZImask));
		if ((ZInode) == NULL) {
			goto ZL3;
		}
	
#line 811 "src/libre/dialect/sql/parser.c"
					}
					/* END OF ACTION: ast-make-subtract */
				}
				break;
			case (ERROR_TERMINAL):
				RESTORE_LEXER;
				goto ZL3;
			default:
				goto ZL3;
			}
			goto ZL2;
		ZL3:;
			{
				/* BEGINNING OF ACTION: err-expected-closegroup */
				{
#line 548 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCLOSEGROUP;
		}
		goto ZL1;
	
#line 834 "src/libre/dialect/sql/parser.c"
				}
				/* END OF ACTION: err-expected-closegroup */
				/* BEGINNING OF ACTION: ast-make-empty */
				{
#line 692 "src/libre/parser.act"

		(ZInode) = ast_make_expr_empty(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 846 "src/libre/dialect/sql/parser.c"
				}
				/* END OF ACTION: ast-make-empty */
			}
		ZL2:;
		}
		/* END OF INLINE: 140 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
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
	
#line 896 "src/libre/dialect/sql/parser.c"
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
	
#line 926 "src/libre/dialect/sql/parser.c"
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
	
#line 947 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: err-expected-alts */
		/* BEGINNING OF ACTION: ast-make-empty */
		{
#line 692 "src/libre/parser.act"

		(ZInode) = ast_make_expr_empty(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL2;
		}
	
#line 959 "src/libre/dialect/sql/parser.c"
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
p_194(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZIclass)
{
	switch (CURRENT_TERMINAL) {
	case (TOK_RANGE):
		{
			t_char ZIc;
			t_pos ZI104;
			t_pos ZI105;
			t_ast__expr ZInode;

			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 237 "src/libre/parser.act"

		ZIc = '-';
		ZI104 = lex_state->lx.start;
		ZI105   = lex_state->lx.end;
	
#line 990 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-make-literal */
			{
#line 713 "src/libre/parser.act"

		(ZInode) = ast_make_expr_literal(act_state->poolp, *flags, (ZIc));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1003 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-literal */
			/* BEGINNING OF ACTION: ast-add-alt */
			{
#line 867 "src/libre/parser.act"

		if (!ast_add_expr_alt((*ZIclass), (ZInode))) {
			goto ZL1;
		}
	
#line 1014 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-add-alt */
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

		(*ZIclass) = ast_make_expr_subtract(act_state->poolp, *flags, any, (*ZIclass));
		if ((*ZIclass) == NULL) {
			goto ZL1;
		}
	
#line 1057 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-invert */
		}
		break;
	default:
		{
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

		(*ZIclass) = ast_make_expr_subtract(act_state->poolp, *flags, any, (*ZIclass));
		if ((*ZIclass) == NULL) {
			goto ZL1;
		}
	
#line 1104 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-invert */
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
p_198(flags flags, lex_state lex_state, act_state act_state, err err, t_char *ZI195, t_pos *ZI196, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	switch (CURRENT_TERMINAL) {
	default:
		{
			/* BEGINNING OF ACTION: ast-make-literal */
			{
#line 713 "src/libre/parser.act"

		(ZInode) = ast_make_expr_literal(act_state->poolp, *flags, (*ZI195));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1135 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-literal */
		}
		break;
	case (TOK_RANGE):
		{
			t_endpoint ZIa;
			t_char ZI123;
			t_pos ZI124;
			t_pos ZI125;
			t_char ZIcz;
			t_pos ZI127;
			t_pos ZIend;
			t_endpoint ZIz;

			/* BEGINNING OF ACTION: ast-range-endpoint-literal */
			{
#line 678 "src/libre/parser.act"

		(ZIa).type = AST_ENDPOINT_LITERAL;
		(ZIa).u.literal.c = (*ZI195);
	
#line 1158 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
			/* BEGINNING OF EXTRACT: RANGE */
			{
#line 237 "src/libre/parser.act"

		ZI123 = '-';
		ZI124 = lex_state->lx.start;
		ZI125   = lex_state->lx.end;
	
#line 1169 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: RANGE */
			ADVANCE_LEXER;
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				/* BEGINNING OF EXTRACT: CHAR */
				{
#line 451 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI127 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;

		ZIcz = lex_state->buf.a[0];
	
#line 1187 "src/libre/dialect/sql/parser.c"
				}
				/* END OF EXTRACT: CHAR */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-range-endpoint-literal */
			{
#line 678 "src/libre/parser.act"

		(ZIz).type = AST_ENDPOINT_LITERAL;
		(ZIz).u.literal.c = (ZIcz);
	
#line 1202 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-range-endpoint-literal */
			/* BEGINNING OF ACTION: mark-range */
			{
#line 595 "src/libre/parser.act"

		mark(&act_state->rangestart, &(*ZI196));
		mark(&act_state->rangeend,   &(ZIend));
	
#line 1212 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: mark-range */
			/* BEGINNING OF ACTION: ast-make-range */
			{
#line 821 "src/libre/parser.act"

		struct ast_pos ast_start, ast_end;
		unsigned char lower, upper;

		AST_POS_OF_LX_POS(ast_start, (*ZI196));
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

		(ZInode) = ast_make_expr_range(act_state->poolp, *flags, &(ZIa), ast_start, &(ZIz), ast_end);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1250 "src/libre/dialect/sql/parser.c"
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
p_201(flags flags, lex_state lex_state, act_state act_state, err err, t_pos *ZI199, t_unsigned *ZIm, t_ast__count *ZOc)
{
	t_ast__count ZIc;

	switch (CURRENT_TERMINAL) {
	case (TOK_CLOSECOUNT):
		{
			t_pos ZI165;
			t_pos ZIend;

			/* BEGINNING OF EXTRACT: CLOSECOUNT */
			{
#line 280 "src/libre/parser.act"

		ZI165 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 1284 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: CLOSECOUNT */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: mark-count */
			{
#line 600 "src/libre/parser.act"

		mark(&act_state->countstart, &(*ZI199));
		mark(&act_state->countend,   &(ZIend));
	
#line 1295 "src/libre/dialect/sql/parser.c"
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

			mark(&act_state->countstart, &(*ZI199));
			mark(&act_state->countend,   &(ZIend));

			goto ZL1;
		}

		AST_POS_OF_LX_POS(ast_start, (*ZI199));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		(ZIc) = ast_make_count((*ZIm), &ast_start, (*ZIm), &ast_end);
	
#line 1320 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: count-range */
		}
		break;
	case (TOK_SEP):
		{
			t_unsigned ZIn;
			t_pos ZI168;
			t_pos ZIend;

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

		ZIn = (unsigned int) u;
	
#line 1356 "src/libre/dialect/sql/parser.c"
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
#line 280 "src/libre/parser.act"

		ZI168 = lex_state->lx.start;
		ZIend   = lex_state->lx.end;
	
#line 1373 "src/libre/dialect/sql/parser.c"
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

		mark(&act_state->countstart, &(*ZI199));
		mark(&act_state->countend,   &(ZIend));
	
#line 1388 "src/libre/dialect/sql/parser.c"
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

			mark(&act_state->countstart, &(*ZI199));
			mark(&act_state->countend,   &(ZIend));

			goto ZL1;
		}

		AST_POS_OF_LX_POS(ast_start, (*ZI199));
		AST_POS_OF_LX_POS(ast_end, (ZIend));

		(ZIc) = ast_make_count((*ZIm), &ast_start, (ZIn), &ast_end);
	
#line 1413 "src/libre/dialect/sql/parser.c"
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
	
#line 1454 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: ast-add-alt */
		/* BEGINNING OF INLINE: 178 */
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
		/* END OF INLINE: 178 */
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
	
#line 1486 "src/libre/dialect/sql/parser.c"
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
			t_pos ZI199;
			t_pos ZI200;
			t_unsigned ZIm;

			/* BEGINNING OF EXTRACT: OPENCOUNT */
			{
#line 275 "src/libre/parser.act"

		ZI199 = lex_state->lx.start;
		ZI200   = lex_state->lx.end;
	
#line 1516 "src/libre/dialect/sql/parser.c"
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
	
#line 1544 "src/libre/dialect/sql/parser.c"
				}
				/* END OF EXTRACT: COUNT */
				break;
			default:
				goto ZL1;
			}
			ADVANCE_LEXER;
			p_201 (flags, lex_state, act_state, err, &ZI199, &ZIm, &ZIc);
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
	
#line 1568 "src/libre/dialect/sql/parser.c"
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
	
#line 1582 "src/libre/dialect/sql/parser.c"
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
	
#line 1596 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: count-zero-or-more */
		}
		break;
	default:
		{
			/* BEGINNING OF ACTION: count-one */
			{
#line 654 "src/libre/parser.act"

		(ZIc) = ast_make_count(1, NULL, 1, NULL);
	
#line 1609 "src/libre/dialect/sql/parser.c"
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
#line 520 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXCOUNT;
		}
		goto ZL2;
	
#line 1629 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: err-expected-count */
		/* BEGINNING OF ACTION: count-one */
		{
#line 654 "src/libre/parser.act"

		(ZIc) = ast_make_count(1, NULL, 1, NULL);
	
#line 1638 "src/libre/dialect/sql/parser.c"
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
#line 620 "src/libre/parser.act"

		/* TODO: or the unicode equivalent */
		(ZIa) = &class_any;
	
#line 1668 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: class-any */
			/* BEGINNING OF ACTION: ast-make-named */
			{
#line 854 "src/libre/parser.act"

		(ZIe) = ast_make_expr_named(act_state->poolp, *flags, (ZIa));
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 1680 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-named */
		}
		break;
	case (TOK_CHAR):
		{
			t_char ZIa;
			t_pos ZI159;
			t_pos ZI160;

			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 451 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI159 = lex_state->lx.start;
		ZI160   = lex_state->lx.end;

		ZIa = lex_state->buf.a[0];
	
#line 1703 "src/libre/dialect/sql/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: ast-make-literal */
			{
#line 713 "src/libre/parser.act"

		(ZIe) = ast_make_expr_literal(act_state->poolp, *flags, (ZIa));
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 1716 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-literal */
		}
		break;
	case (TOK_MANY):
		{
			t_ast__class__id ZIa;
			t_ast__expr ZIg;
			t_ast__count ZIc;

			ADVANCE_LEXER;
			/* BEGINNING OF ACTION: class-any */
			{
#line 620 "src/libre/parser.act"

		/* TODO: or the unicode equivalent */
		(ZIa) = &class_any;
	
#line 1735 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: class-any */
			/* BEGINNING OF ACTION: ast-make-named */
			{
#line 854 "src/libre/parser.act"

		(ZIg) = ast_make_expr_named(act_state->poolp, *flags, (ZIa));
		if ((ZIg) == NULL) {
			goto ZL1;
		}
	
#line 1747 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-named */
			/* BEGINNING OF ACTION: count-zero-or-more */
			{
#line 642 "src/libre/parser.act"

		(ZIc) = ast_make_count(0, NULL, AST_COUNT_UNBOUNDED, NULL);
	
#line 1756 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: count-zero-or-more */
			/* BEGINNING OF ACTION: ast-make-piece */
			{
#line 724 "src/libre/parser.act"

		if ((ZIc).min == 0 && (ZIc).max == 0) {
			(ZIe) = ast_make_expr_empty(act_state->poolp, *flags);
		} else if ((ZIc).min == 1 && (ZIc).max == 1) {
			(ZIe) = (ZIg);
		} else {
			(ZIe) = ast_make_expr_repeat(act_state->poolp, *flags, (ZIg), (ZIc));
		}
		if ((ZIe) == NULL) {
			err->e = RE_EXEOF;
			goto ZL1;
		}
	
#line 1775 "src/libre/dialect/sql/parser.c"
			}
			/* END OF ACTION: ast-make-piece */
		}
		break;
	case (TOK_OPENSUB):
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
#line 738 "src/libre/parser.act"

		(ZIe) = ast_make_expr_group(act_state->poolp, *flags, (ZIg));
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 1799 "src/libre/dialect/sql/parser.c"
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
	case (TOK_OPENGROUP):
		{
			p_expr_C_Ccharacter_Hclass (flags, lex_state, act_state, err, &ZIe);
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
	
#line 1837 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: err-expected-atom */
		/* BEGINNING OF ACTION: ast-make-empty */
		{
#line 692 "src/libre/parser.act"

		(ZIe) = ast_make_expr_empty(act_state->poolp, *flags);
		if ((ZIe) == NULL) {
			goto ZL2;
		}
	
#line 1849 "src/libre/dialect/sql/parser.c"
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
p_expr_C_Ccharacter_Hclass_C_Cclass_Hnamed(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__class__id ZIid;
		t_pos ZI115;
		t_pos ZI116;

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

		ZI115 = lex_state->lx.start;
		ZI116   = lex_state->lx.end;
	
#line 1889 "src/libre/dialect/sql/parser.c"
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
	
#line 1906 "src/libre/dialect/sql/parser.c"
		}
		/* END OF ACTION: ast-make-named */
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
			/* BEGINNING OF ACTION: ast-make-concat */
			{
#line 699 "src/libre/parser.act"

		(ZInode) = ast_make_expr_concat(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 1936 "src/libre/dialect/sql/parser.c"
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
	
#line 1957 "src/libre/dialect/sql/parser.c"
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

#line 2117 "src/libre/dialect/sql/parser.c"

/* END OF FILE */
