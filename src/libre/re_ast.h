/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_AST_H
#define RE_AST_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <re/re.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>

#include "re_char_class.h"

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
	AST_EXPR_CONCAT_N,
	AST_EXPR_ALT,
	AST_EXPR_ALT_N,
	AST_EXPR_LITERAL,
	AST_EXPR_ANY,
	AST_EXPR_REPEATED,
	AST_EXPR_CHAR_CLASS,
	AST_EXPR_GROUP,
	AST_EXPR_FLAGS,
	AST_EXPR_ANCHOR,
	AST_EXPR_TOMBSTONE
};

#define AST_COUNT_UNBOUNDED ((unsigned)-1)
struct ast_count {
	unsigned low;
	struct ast_pos start;
	unsigned high;
	struct ast_pos end;
};

enum re_ast_anchor_type {
	RE_AST_ANCHOR_START,
	RE_AST_ANCHOR_END
};

/* Flags used during AST analysis. Not all are valid for all node types. */
enum re_ast_flags {
	/* The node can appear at the beginning of input,
	 * possibly preceded by other nullable nodes. */
	RE_AST_FLAG_FIRST_STATE = 0x01,

	/* This node can appear at the end of input, possibly
	 * followed by nullable nodes. */
	RE_AST_FLAG_LAST_STATE = 0x02,

	/* The node caused the regex to become unsatisfiable. */
	RE_AST_FLAG_UNSATISFIABLE = 0x04,

	/* The node is not always evaluated, such as nodes that
	 * are repeated at least 0 times. */
	RE_AST_FLAG_NULLABLE = 0x08,

	RE_AST_FLAG_NONE = 0x00
};

#define NO_GROUP_ID ((unsigned)-1)

/*
 * The following regular expression fragments map to associated fsm states
 * as follows (transitions written in .fsm format):
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
	enum re_ast_flags flags;

	union {
		struct {
			struct ast_expr *l;
			struct ast_expr *r;
		} concat;
		struct {
			size_t count;
			struct ast_expr *n[1];
		} concat_n;
		struct {
			struct ast_expr *l;
			struct ast_expr *r;
		} alt;
		struct {
			size_t count;
			struct ast_expr *n[1];
		} alt_n;
		struct {
			/*const*/ char c;
		} literal;
		struct ast_expr_repeated {
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
			struct ast_expr *e;
			unsigned id;
		} group;
		struct {
			enum re_flags pos;
			enum re_flags neg;
		} flags;
		struct {
			enum re_ast_anchor_type t;
		} anchor;
	} u;	
};

struct ast_re {
	struct ast_expr *expr;
	/* This is set if we can determine ahead of time that the regex
	 * is inherently unsatisfiable. For example, multiple start/end
	 * anchors that aren't in nullable groups or different alts.
	 * While this naming leads to a double-negative, we can't prove
	 * that the regex is satisfiable, just flag it when it isn't. */
	int unsatisfiable;
};

extern struct ast_expr the_empty_node;

struct ast_re *
re_ast_new(void);

void
re_ast_free(struct ast_re *ast);

void
re_ast_expr_free(struct ast_expr *n);

struct ast_expr *
re_ast_expr_empty(void);

struct ast_expr *
re_ast_expr_tombstone(void);

struct ast_expr *
re_ast_expr_concat(struct ast_expr *l, struct ast_expr *r);

struct ast_expr *
re_ast_expr_concat_n(size_t count);

struct ast_expr *
re_ast_expr_alt(struct ast_expr *l, struct ast_expr *r);

struct ast_expr *
re_ast_expr_alt_n(size_t count);

struct ast_expr *
re_ast_expr_literal(char c);

struct ast_expr *
re_ast_expr_any(void);

struct ast_expr *
re_ast_expr_with_count(struct ast_expr *e, struct ast_count count);

struct ast_expr *
re_ast_expr_char_class(struct re_char_class_ast *cca,
    const struct ast_pos *start, const struct ast_pos *end);

struct ast_expr *
re_ast_expr_group(struct ast_expr *e);

struct ast_expr *
re_ast_expr_re_flags(enum re_flags pos, enum re_flags neg);

struct ast_expr *
re_ast_expr_anchor(enum re_ast_anchor_type t);

struct ast_count
ast_count(unsigned low, const struct ast_pos *start,
    unsigned high, const struct ast_pos *end);

#endif
