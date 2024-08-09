/*
 * Automatically generated from the files:
 *	src/libre/dialect/literal/parser.sid
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

#line 273 "src/libre/dialect/literal/parser.c"


#ifndef ERROR_TERMINAL
#error "-s no-numeric-terminals given and ERROR_TERMINAL is not defined"
#endif

/* BEGINNING OF FUNCTION DECLARATIONS */

static void p_list_Hof_Hatoms(flags, lex_state, act_state, err, t_ast__expr);
extern void p_re__literal(flags, lex_state, act_state, err, t_ast__expr *);
static void p_list_Hof_Hatoms_C_Catom(flags, lex_state, act_state, err, t_ast__expr *);

/* BEGINNING OF STATIC VARIABLES */


/* BEGINNING OF FUNCTION DEFINITIONS */

static void
p_list_Hof_Hatoms(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr ZIcat)
{
	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
ZL2_list_Hof_Hatoms:;
	{
		t_ast__expr ZIa;

		p_list_Hof_Hatoms_C_Catom (flags, lex_state, act_state, err, &ZIa);
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
	
#line 314 "src/libre/dialect/literal/parser.c"
		}
		/* END OF ACTION: ast-add-concat */
		/* BEGINNING OF INLINE: 114 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					/* BEGINNING OF INLINE: list-of-atoms */
					goto ZL2_list_Hof_Hatoms;
					/* END OF INLINE: list-of-atoms */
				}
				/*UNREACHED*/
			default:
				break;
			}
		}
		/* END OF INLINE: 114 */
	}
	return;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
}

void
p_re__literal(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
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
	
#line 357 "src/libre/dialect/literal/parser.c"
		}
		/* END OF ACTION: make-group-id */
		/* BEGINNING OF INLINE: 117 */
		{
			switch (CURRENT_TERMINAL) {
			case (TOK_CHAR):
				{
					/* BEGINNING OF ACTION: ast-make-concat */
					{
#line 861 "src/libre/parser.act"

		(ZIe) = ast_make_expr_concat(act_state->poolp, *flags);
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 374 "src/libre/dialect/literal/parser.c"
					}
					/* END OF ACTION: ast-make-concat */
					p_list_Hof_Hatoms (flags, lex_state, act_state, err, ZIe);
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

		(ZIe) = ast_make_expr_empty(act_state->poolp, *flags);
		if ((ZIe) == NULL) {
			goto ZL1;
		}
	
#line 395 "src/libre/dialect/literal/parser.c"
					}
					/* END OF ACTION: ast-make-empty */
				}
				break;
			}
		}
		/* END OF INLINE: 117 */
		/* BEGINNING OF ACTION: ast-make-group */
		{
#line 912 "src/libre/parser.act"

		(ZInode) = ast_make_expr_group(act_state->poolp, *flags, (ZIe), (ZIid));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 412 "src/libre/dialect/literal/parser.c"
		}
		/* END OF ACTION: ast-make-group */
		/* BEGINNING OF INLINE: 119 */
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
#line 757 "src/libre/parser.act"

		if (err->e == RE_ESUCCESS) {
			err->e = RE_EXEOF;
		}
		goto ZL1;
	
#line 438 "src/libre/dialect/literal/parser.c"
				}
				/* END OF ACTION: err-expected-eof */
			}
		ZL3:;
		}
		/* END OF INLINE: 119 */
	}
	goto ZL0;
ZL1:;
	SAVE_LEXER ((ERROR_TERMINAL));
	return;
ZL0:;
	*ZOnode = ZInode;
}

static void
p_list_Hof_Hatoms_C_Catom(flags flags, lex_state lex_state, act_state act_state, err err, t_ast__expr *ZOnode)
{
	t_ast__expr ZInode;

	if ((CURRENT_TERMINAL) == (ERROR_TERMINAL)) {
		return;
	}
	{
		t_char ZIc;
		t_pos ZI111;
		t_pos ZI112;

		switch (CURRENT_TERMINAL) {
		case (TOK_CHAR):
			/* BEGINNING OF EXTRACT: CHAR */
			{
#line 579 "src/libre/parser.act"

		/* the first byte may be '\x00' */
		assert(lex_state->buf.a[1] == '\0');

		ZI111 = lex_state->lx.start;
		ZI112   = lex_state->lx.end;

		(void) ZI111;
		(void) ZI112;

		ZIc = lex_state->buf.a[0];
	
#line 484 "src/libre/dialect/literal/parser.c"
			}
			/* END OF EXTRACT: CHAR */
			break;
		default:
			goto ZL1;
		}
		ADVANCE_LEXER;
		/* BEGINNING OF ACTION: ast-make-literal */
		{
#line 875 "src/libre/parser.act"

		(ZInode) = ast_make_expr_literal(act_state->poolp, *flags, (ZIc));
		if ((ZInode) == NULL) {
			goto ZL1;
		}
	
#line 501 "src/libre/dialect/literal/parser.c"
		}
		/* END OF ACTION: ast-make-literal */
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
	
#line 517 "src/libre/dialect/literal/parser.c"
		}
		/* END OF ACTION: err-expected-atom */
		/* BEGINNING OF ACTION: ast-make-empty */
		{
#line 854 "src/libre/parser.act"

		(ZInode) = ast_make_expr_empty(act_state->poolp, *flags);
		if ((ZInode) == NULL) {
			goto ZL2;
		}
	
#line 529 "src/libre/dialect/literal/parser.c"
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

#line 699 "src/libre/dialect/literal/parser.c"

/* END OF FILE */
