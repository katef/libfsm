/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_AST_H
#define RE_AST_H

/*
 * This is a duplicate of struct lx_pos, but since we're linking to
 * code with several distinct lexers, there isn't a clear lexer.h
 * to include here. The parser sees both definitions, and will
 * build a `struct ast_pos` in the appropriate places.
 */
struct ast_pos {
	unsigned byte;
	unsigned line;
	unsigned col;
};

enum ast_expr_type {
	AST_EXPR_EMPTY,
	AST_EXPR_CONCAT,
	AST_EXPR_ALT,
	AST_EXPR_LITERAL,
	AST_EXPR_ANY,
	AST_EXPR_REPEATED,
	AST_EXPR_CLASS,
	AST_EXPR_GROUP,
	AST_EXPR_FLAGS,
	AST_EXPR_ANCHOR,
	AST_EXPR_SUBTRACT,
	AST_EXPR_INVERT,
	AST_EXPR_TOMBSTONE
};

#define AST_COUNT_UNBOUNDED ((unsigned)-1)
struct ast_count {
	unsigned low;
	struct ast_pos start;
	unsigned high;
	struct ast_pos end;
};

enum ast_anchor_type {
	AST_ANCHOR_START,
	AST_ANCHOR_END
};

/*
 * Flags used during AST analysis for expression nodes:
 *
 * - AST_EXPR_FLAG_FIRST
 *   The node can appear at the beginning of input,
 *   possibly preceded by other nullable nodes.
 *
 * - AST_EXPR_FLAG_LAST
 *   This node can appear at the end of input, possibly
 *   followed by nullable nodes.
 *
 * - AST_EXPR_FLAG_UNSATISFIABLE
 *   The node caused the regex to become unsatisfiable.
 *
 * - AST_EXPR_FLAG_NULLABLE
 *   The node is not always evaluated, such as nodes that
 *   are repeated at least 0 times.
 *
 * Not all are valid for all node types.
 */
enum ast_expr_flags {
	AST_EXPR_FLAG_FIRST         = 1 << 0,
	AST_EXPR_FLAG_LAST          = 1 << 1,
	AST_EXPR_FLAG_UNSATISFIABLE = 1 << 2,
	AST_EXPR_FLAG_NULLABLE      = 1 << 3,

	AST_EXPR_FLAG_NONE = 0x00
};

#define NO_GROUP_ID ((unsigned)-1)

enum ast_class_type {
	AST_CLASS_CONCAT,
	AST_CLASS_LITERAL,
	AST_CLASS_RANGE,
	AST_CLASS_NAMED
/*	AST_CLASS_TYPE XXX: not implemented */
};

enum ast_endpoint_type {
	AST_ENDPOINT_LITERAL,
/*	AST_ENDPOINT_TYPE, XXX: not implemented */
	AST_ENDPOINT_CLASS
};

struct ast_endpoint {
	enum ast_endpoint_type type;
	union {
		struct {
			unsigned char c;
		} literal;

		struct {
			class_constructor *ctor;
		} class;
	} u;
};

struct ast_class {
	enum ast_class_type type;
	union {
		struct {
			unsigned char c;
		} literal;

		struct {
			struct ast_endpoint from;
			struct ast_pos start;
			struct ast_endpoint to;
			struct ast_pos end;
		} range;

		struct {
			class_constructor *ctor;
		} named;
	} u;
};

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
	enum ast_expr_type type;
	enum ast_expr_flags flags;

	union {
		/* ordered sequence */
		struct {
			size_t count; /* used */
			size_t alloc; /* allocated */
			struct ast_expr **n;
		} concat;

		/* unordered set */
		struct {
			size_t count; /* used */
			size_t alloc; /* allocated */
			struct ast_expr **n;
		} alt;

		struct {
			/*const*/ char c;
		} literal;

		struct ast_expr_repeated {
			struct ast_expr *e;
			unsigned low;
			unsigned high;
		} repeated;

		/* unordered set */
		struct {
			size_t count; /* used */
			size_t alloc; /* allocated */
			struct ast_class **n;

			struct ast_pos start;
			struct ast_pos end;
		} class;

		struct {
			struct ast_expr *e;
			unsigned id;
		} group;

		struct {
			enum re_flags pos;
			enum re_flags neg;
		} flags;

		struct {
			enum ast_anchor_type type;
		} anchor;

		struct ast_expr_subtract {
			struct ast_expr *a;
			struct ast_expr *b;
		} subtract;

		struct ast_expr_invert {
			struct ast_expr *e;
		} invert;
	} u;	
};

struct ast {
	struct ast_expr *expr;
};

struct ast_expr *ast_expr_tombstone;

struct ast *
ast_new(void);

void
ast_free(struct ast *ast);

struct ast_count
ast_make_count(unsigned low, const struct ast_pos *start,
	unsigned high, const struct ast_pos *end);

/*
 * Expressions
 */

void
ast_expr_free(struct ast_expr *n);

struct ast_expr *
ast_make_expr_empty(void);

struct ast_expr *
ast_make_expr_concat(void);

struct ast_expr *
ast_make_expr_alt(void);

int
ast_add_expr_alt(struct ast_expr *cat, struct ast_expr *node);

struct ast_expr *
ast_make_expr_literal(char c);

struct ast_expr *
ast_make_expr_any(void);

struct ast_expr *
ast_make_expr_with_count(struct ast_expr *e, struct ast_count count);

struct ast_expr *
ast_make_expr_class(void);

struct ast_expr *
ast_make_expr_group(struct ast_expr *e);

struct ast_expr *
ast_make_expr_re_flags(enum re_flags pos, enum re_flags neg);

struct ast_expr *
ast_make_expr_anchor(enum ast_anchor_type type);

struct ast_expr *
ast_make_expr_subtract(struct ast_expr *a, struct ast_expr *b);

struct ast_expr *
ast_make_expr_invert(struct ast_expr *e);

int
ast_add_expr_concat(struct ast_expr *cat, struct ast_expr *node);

/*
 * Character classes
 */

struct ast_class *
ast_make_class_literal(unsigned char c);

struct ast_class *
ast_make_class_range(const struct ast_endpoint *from, struct ast_pos start,
	const struct ast_endpoint *to, struct ast_pos end);

struct ast_class *
ast_make_class_named(class_constructor *ctor);

int
ast_add_class_concat(struct ast_expr *class, struct ast_class *node);

/* XXX: exposed for sake of re(1) printing an ast;
 * it's not part of the <re/re.h> API proper */
struct fsm_options;
struct ast *
re_parse(enum re_dialect dialect, int (*getc)(void *opaque), void *opaque,
	const struct fsm_options *opt,
	enum re_flags flags, struct re_err *err, int *unsatisfiable);

#endif
