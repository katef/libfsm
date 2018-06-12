#ifndef RE_AST_H
#define RE_AST_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <re/re.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>

#include "re_class.h"

#if 1
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...)
#endif

/* Because we're building with -NDEBUG :( */
#define FAIL(s)					\
	do {					\
		fprintf(stderr, s);		\
		abort();			\
	} while(0)

/* Parse tree / Abstract syntax tree.
 * None of this should be exposed to the public API. */

/* DIALECTS, in general complexity order :
 * - literal
 * - glob
 * ???
 * - like
 * - sql
 * - native
 * ???
 * - pcre
 * */

/* == literal == */

/* fwd refs */

struct ast_re;			/* toplevel struct */

struct ast_expr;		/* expression: main recursive node */
/* struct ast_flags; */
/* struct ast_group; */
struct ast_literal;
/* struct ast_class; */
/* struct ast_count; */

struct ast_re {
	struct ast_expr *expr;
};


struct ast_literal {
	/*const*/ char c;
};

enum ast_class_id {
	AST_CLASS_ALNUM,
	AST_CLASS_ALPHA,
	AST_CLASS_ANY,
	AST_CLASS_ASCII,
	AST_CLASS_BLANK,
	AST_CLASS_CNTRL,
	AST_CLASS_DIGIT,
	AST_CLASS_GRAPH,
	AST_CLASS_LOWER,
	AST_CLASS_PRINT,
	AST_CLASS_PUNCT,
	AST_CLASS_SPACE,
	AST_CLASS_SPCHR,
	AST_CLASS_UPPER,
	AST_CLASS_WORD,
	AST_CLASS_XDIGIT
};

enum ast_expr_type {
	AST_EXPR_EMPTY,
	AST_EXPR_CONCAT,
	AST_EXPR_ALT,
	AST_EXPR_LITERAL,
	AST_EXPR_ANY,
	AST_EXPR_MANY,
	AST_EXPR_KLEENE,
	AST_EXPR_PLUS,
	AST_EXPR_OPT,
	AST_EXPR_REPEATED,
	AST_EXPR_CLASS
};

#define AST_COUNT_UNBOUNDED ((unsigned)-1)
struct ast_count {
	unsigned low;
	unsigned high;
};

struct ast_expr {
	enum ast_expr_type t;
	union {
		struct {
			struct ast_expr *l;
			struct ast_expr *r;
		} concat;
		struct {
			struct ast_expr *l;
			struct ast_expr *r;
		} alt;
		struct {
			struct ast_literal l;
		} literal;
		struct {
			struct ast_expr *e;
		} kleene;
		struct {
			struct ast_expr *e;
		} plus;
		struct {
			struct ast_expr *e;
		} opt;
		struct {
			struct ast_expr *e;
			unsigned low;
			unsigned high;
		} repeated;
		struct {
			struct ast_char_class *cc;
		} class;
	} u;	
};

struct ast_re *
re_ast_new(void);

void
re_ast_free(struct ast_re *ast);

struct ast_expr *
re_ast_expr_empty(void);

struct ast_expr *
re_ast_expr_concat(struct ast_expr *l, struct ast_expr *r);

struct ast_expr *
re_ast_expr_alt(struct ast_expr *l, struct ast_expr *r);

struct ast_expr *
re_ast_expr_literal(char c);

struct ast_expr *
re_ast_expr_any(void);

struct ast_expr *
re_ast_expr_many(void);

struct ast_expr *
re_ast_expr_with_count(struct ast_expr *e, struct ast_count count);

struct ast_expr *
re_ast_expr_class(enum ast_char_class_flags flags, struct ast_char_class *cc);

void
re_ast_prettyprint(FILE *f, struct ast_re *ast);

struct ast_count
ast_count(unsigned low, unsigned high);

#endif
