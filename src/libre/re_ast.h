#ifndef RE_AST_H
#define RE_AST_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <re/re.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>

#include "re_char_class.h"

#define PRETTYPRINT_AST 0
#define LOGGING 0

#if LOGGING
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...)
#endif

/* Because we're building with -NDEBUG */
#define FAIL(s)					\
	do {					\
		fprintf(stderr, s);		\
		abort();			\
	} while(0)

/* Parse tree / Abstract syntax tree.
 * None of this should be exposed to the public API. */

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
	AST_EXPR_CHAR_CLASS,
	AST_EXPR_CHAR_TYPE,
	AST_EXPR_GROUP,
	AST_EXPR_FLAGS
};

#define AST_COUNT_UNBOUNDED ((unsigned)-1)
struct ast_count {
	unsigned low;
	struct ast_pos start;
	unsigned high;
	struct ast_pos end;
};

/*
 * The following regular expression fragments map to associated fsm states
= * as follows (transitions written in .fsm format):
 *
 *  ab    concat:      1 -> 3 "a"; 3 -> 2 "b";
 *  a|b   alt:         1 -> 2 "a"; 1 -> 2 "b";
 *  a     literal:     1 -> 1a; 2a -> 2;
 *  .     any:         1 -> 2 ?;
 *  (a)   group:       1 -> 1a; 2a -> 2;
 *  [abc] char-class:  1 -> 2 "a"; 1 -> 2 "b"; 1 -> 2 "c";
 */
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
			/*const*/ char c;
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
			struct re_char_class_ast *cca;
			struct ast_pos start;
			struct ast_pos end;
		} char_class;
		struct {
			enum ast_char_type_id id;
		} char_type;
		struct {
			struct ast_expr *e;
			unsigned id;
		} group;
		struct {
			enum re_flags pos;
			enum re_flags neg;
			/* Previous flags, saved here and restored
			 * when done compiling the flags node's subtree.
			 * Since `struct ast_expr` contains a union,
			 * this space is already allocated. */
			enum re_flags saved;
		} flags;
	} u;	
};

struct ast_re {
	struct ast_expr *expr;
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
re_ast_expr_char_class(struct re_char_class_ast *cca,
    const struct ast_pos *start, const struct ast_pos *end);

struct ast_expr *
re_ast_expr_char_type(enum ast_char_type_id id);

struct ast_expr *
re_ast_expr_group(struct ast_expr *e);

struct ast_expr *
re_ast_expr_re_flags(enum re_flags pos, enum re_flags neg);

void
re_ast_prettyprint(FILE *f, struct ast_re *ast);

struct ast_count
ast_count(unsigned low, const struct ast_pos *start,
    unsigned high, const struct ast_pos *end);

#endif
