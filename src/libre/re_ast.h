#ifndef RE_AST_H
#define RE_AST_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <re/re.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>

#if 1
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...)
#endif


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


enum ast_expr_type {
	AST_EXPR_EMPTY,
	AST_EXPR_LITERAL,
	AST_EXPR_ANY,
	AST_EXPR_MANY
};

struct ast_expr {
	enum ast_expr_type t;
	union {
		struct {
			struct ast_literal l;
			struct ast_expr *n;
		} literal;
		struct {
			struct ast_expr *n;
		} any;
		struct {
			struct ast_expr *n;
		} many;
	} u;	
};

struct ast_re *
re_ast_new(void);

void
re_ast_free(struct ast_re *ast);

struct ast_expr *
re_ast_expr_empty(void);

struct ast_expr *
re_ast_expr_literal(char c, struct ast_expr *r);

struct ast_expr *
re_ast_expr_any(struct ast_expr *r);

struct ast_expr *
re_ast_expr_many(struct ast_expr *r);

void
re_ast_prettyprint(FILE *f, struct ast_re *ast);

#endif
