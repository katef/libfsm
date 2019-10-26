/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_AST_CLASS_H
#define RE_AST_CLASS_H

struct fsm;
struct fsm_state;
struct fsm_options;

enum ast_class_flags {
	AST_CLASS_FLAG_NONE = 0x00,
	/* the class should be negated, e.g. [^aeiou] */
	AST_CLASS_FLAG_INVERTED = 0x01,
	/* includes the `-` character, which isn't part of a range */
	AST_CLASS_FLAG_MINUS = 0x02
};

enum ast_class_type {
	AST_CLASS_CONCAT,
	AST_CLASS_LITERAL,
	AST_CLASS_RANGE,
	AST_CLASS_NAMED,
	AST_CLASS_CHAR_TYPE,
	AST_CLASS_FLAGS,
	AST_CLASS_SUBTRACT
};

enum ast_range_endpoint_type {
	AST_RANGE_ENDPOINT_LITERAL,
	AST_RANGE_ENDPOINT_CHAR_TYPE,
	AST_RANGE_ENDPOINT_CHAR_CLASS
};
struct ast_range_endpoint {
	enum ast_range_endpoint_type t;
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
	enum ast_class_type t;
	union {
		struct {
			struct ast_class *l;
			struct ast_class *r;
		} concat;
		struct {
			unsigned char c;
		} literal;
		struct {
			struct ast_range_endpoint from;
			struct ast_pos start;
			struct ast_range_endpoint to;
			struct ast_pos end;
		} range;
		struct {
			class_constructor *ctor;
		} named;
		struct {
			enum ast_class_flags f;
		} flags;
		struct {
			struct ast_class *ast;
			struct ast_class *mask;
		} subtract;
	} u;
};

struct ast_class *
ast_class_concat(struct ast_class *l,
    struct ast_class *r);

struct ast_class *
ast_class_literal(unsigned char c);

struct ast_class *
ast_class_range(const struct ast_range_endpoint *from, struct ast_pos start,
    const struct ast_range_endpoint *to, struct ast_pos end);

void
ast_class_endpoint_span(const struct ast_range_endpoint *r,
    unsigned char *from, unsigned char *to);

struct ast_class *
ast_class_flags(enum ast_class_flags flags);

struct ast_class *
ast_class_named(class_constructor *ctor);

struct ast_class *
ast_class_subtract(struct ast_class *ast,
    struct ast_class *mask);

void
ast_class_free(struct ast_class *ast);

int
ast_class_compile(struct ast_class *class,
    struct fsm *fsm, enum re_flags flags,
    struct re_err *err, const struct fsm_options *opt,
    struct fsm_state *x, struct fsm_state *y);

#endif
