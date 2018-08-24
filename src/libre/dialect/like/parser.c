/*
 * Automatically generated from the files:
 *	src/libre/dialect/like/parser.sid
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

#line 219 "src/libre/dialect/like/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Hnodes(flags, lex_state, act_state, err, t_ast__expr *);
extern void p_re__like(flags, lex_state, act_state, err, t_ast__expr *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Hnodes(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_ast__expr ZIl;

		/* BEGINNING OF INLINE: 101 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY):
				{
					/* BEGINNING OF INLINE: list-of-nodes::any */
					{
						{
							switch (CURRENT_TERMINAL) {
							case (TOK_ANY):
								break;
							default:
								goto ZL1;
							}
							ADVANCE_LEXER;
						}
					}
					/* END OF INLINE: list-of-nodes::any */
					/* BEGINNING OF ACTION: ast-expr-any */
					{
#line 579 "src/libre/parser.act"

		(ZIl) = re_ast_expr_any();
		if ((ZIl) == NULL) { goto ZL1; }
	
#line 272 "src/libre/dialect/like/parser.c"
					}
					/* END OF ACTION: ast-expr-any */
				}
				break;
			case (TOK_CHAR):
				{
					t_char ZIc;

					/* BEGINNING OF INLINE: list-of-nodes::literal */
					{
						{
							t_pos ZI95;
							t_pos ZI96;

							switch (CURRENT_TERMINAL) {
							case (TOK_CHAR):
								/* BEGINNING OF EXTRACT: CHAR */
								{
#line 401 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI95 = lex_state->lx.start;
		ZI96   = lex_state->lx.end;

		ZIc = lex_state->buf.a[0];
	
#line 301 "src/libre/dialect/like/parser.c"
								}
								/* END OF EXTRACT: CHAR */
								break;
							default:
								goto ZL1;
							}
							ADVANCE_LEXER;
						}
					}
					/* END OF INLINE: list-of-nodes::literal */
					/* BEGINNING OF ACTION: ast-expr-literal */
					{
#line 574 "src/libre/parser.act"

		(ZIl) = re_ast_expr_literal((ZIc));
		if ((ZIl) == NULL) { goto ZL1; }
	
#line 319 "src/libre/dialect/like/parser.c"
					}
					/* END OF ACTION: ast-expr-literal */
				}
				break;
			case (TOK_MANY):
				{
					t_ast__expr ZIe;
					t_ast__count ZIc;

					/* BEGINNING OF INLINE: list-of-nodes::many */
					{
						{
							switch (CURRENT_TERMINAL) {
							case (TOK_MANY):
								break;
							default:
								goto ZL1;
							}
							ADVANCE_LEXER;
						}
					}
					/* END OF INLINE: list-of-nodes::many */
					/* BEGINNING OF ACTION: ast-expr-any */
					{
#line 579 "src/libre/parser.act"

		(ZIe) = re_ast_expr_any();
		if ((ZIe) == NULL) { goto ZL1; }
	
#line 349 "src/libre/dialect/like/parser.c"
					}
					/* END OF ACTION: ast-expr-any */
					/* BEGINNING OF ACTION: atom-kleene */
					{
#line 604 "src/libre/parser.act"

		(ZIc) = ast_count(0, NULL, AST_COUNT_UNBOUNDED, NULL);
	
#line 358 "src/libre/dialect/like/parser.c"
					}
					/* END OF ACTION: atom-kleene */
					/* BEGINNING OF ACTION: ast-expr-atom */
					{
#line 596 "src/libre/parser.act"

		(ZIl) = re_ast_expr_with_count((ZIe), (ZIc));
		if ((ZIl) == NULL) {
			err->e = RE_EXEOF;
			goto ZL1;
		}
	
#line 371 "src/libre/dialect/like/parser.c"
					}
					/* END OF ACTION: ast-expr-atom */
				}
				break;
			default:
				goto ZL1;
			}
		}
		/* END OF INLINE: 101 */
		/* BEGINNING OF INLINE: 104 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY): case (TOK_MANY): case (TOK_CHAR):
				{
					t_ast__expr ZIr;

					p_list_Hof_Hnodes (flags, lex_state, act_state, err, &ZIr);
					if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
						RESTORE_LEXER;
						goto ZL1;
					}
					/* BEGINNING OF ACTION: ast-expr-concat */
					{
#line 564 "src/libre/parser.act"

		(ZInode) = re_ast_expr_concat((ZIl), (ZIr));
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 400 "src/libre/dialect/like/parser.c"
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
	
#line 416 "src/libre/dialect/like/parser.c"
					}
					/* END OF ACTION: ast-expr-empty */
					/* BEGINNING OF ACTION: ast-expr-concat */
					{
#line 564 "src/libre/parser.act"

		(ZInode) = re_ast_expr_concat((ZIl), (ZIr));
		if ((ZInode) == NULL) { goto ZL1; }
	
#line 426 "src/libre/dialect/like/parser.c"
					}
					/* END OF ACTION: ast-expr-concat */
				}
				break;
			}
		}
		/* END OF INLINE: 104 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

void
p_re__like(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		/* BEGINNING OF INLINE: 107 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_ANY): case (TOK_MANY): case (TOK_CHAR):
				{
					p_list_Hof_Hnodes (flags, lex_state, act_state, err, &ZInode);
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
	
#line 473 "src/libre/dialect/like/parser.c"
					}
					/* END OF ACTION: ast-expr-empty */
				}
				break;
			}
		}
		/* END OF INLINE: 107 */
		/* BEGINNING OF INLINE: 108 */
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
	
#line 504 "src/libre/dialect/like/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL3:;
		}
		/* END OF INLINE: 108 */
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

#line 666 "src/libre/dialect/like/parser.c"

/* END OF FILE */
