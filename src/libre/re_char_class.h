/*
 * Copyright 2018 Scott Vokes
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_CHAR_CLASS_H
#define RE_CHAR_CLASS_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/pred.h>

#include <re/re.h>

/* This is a duplicate of struct lx_pos, but since we're linking to
 * code with several distinct lexers, there isn't a clear lexer.h
 * to include here. The parser sees both definitions, and will
 * build a `struct ast_pos` in the appropriate places. */
struct ast_pos {
	unsigned byte;
	unsigned line;
	unsigned col;
};


/* typedef for character class constructors whose prototypes
 * appear in class.h -- this typedef should be moved there
 * later. */
typedef struct fsm *
char_class_constructor_fun(const struct fsm_options *opt);

/* For a particular regex dialect, this function will look up
 * whether a name refers to a known character class, and if so,
 * return the constructor function for it.
 * Returns whether lookup suceeded. */
typedef int
re_dialect_char_class_fun(const char *name, char_class_constructor_fun **res);

enum re_char_class_flags {
	RE_CHAR_CLASS_FLAG_NONE = 0x00,
	/* the class should be negated, e.g. [^aeiou] */
	RE_CHAR_CLASS_FLAG_INVERTED = 0x01,
	/* includes the `-` character, which isn't part of a range */
	RE_CHAR_CLASS_FLAG_MINUS = 0x02
};

enum re_char_class_ast_type {
	RE_CHAR_CLASS_AST_CONCAT,
	RE_CHAR_CLASS_AST_LITERAL,
	RE_CHAR_CLASS_AST_RANGE,
	RE_CHAR_CLASS_AST_NAMED,
	RE_CHAR_CLASS_AST_CHAR_TYPE,
	RE_CHAR_CLASS_AST_FLAGS,
	RE_CHAR_CLASS_AST_SUBTRACT
};

/* These are used for named character classes, such as [:alnum:]. */
enum ast_char_class_id {
	AST_CHAR_CLASS_ALNUM,
	AST_CHAR_CLASS_ALPHA,
	AST_CHAR_CLASS_ANY,
	AST_CHAR_CLASS_ASCII,
	AST_CHAR_CLASS_BLANK,
	AST_CHAR_CLASS_CNTRL,
	AST_CHAR_CLASS_DIGIT,
	AST_CHAR_CLASS_GRAPH,
	AST_CHAR_CLASS_LOWER,
	AST_CHAR_CLASS_PRINT,
	AST_CHAR_CLASS_PUNCT,
	AST_CHAR_CLASS_SPACE,
	AST_CHAR_CLASS_SPCHR,
	AST_CHAR_CLASS_UPPER,
	AST_CHAR_CLASS_WORD,
	AST_CHAR_CLASS_XDIGIT
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
			char_class_constructor_fun *ctor;
		} char_class;
	} u;
};

struct re_char_class_ast {
	enum re_char_class_ast_type t;
	union {
		struct {
			struct re_char_class_ast *l;
			struct re_char_class_ast *r;
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
			char_class_constructor_fun *ctor;
		} named;
		struct {
			enum re_char_class_flags f;
		} flags;
		struct {
			struct re_char_class_ast *ast;
			struct re_char_class_ast *mask;
		} subtract;
	} u;
};

struct re_char_class_ast *
re_char_class_ast_concat(struct re_char_class_ast *l,
    struct re_char_class_ast *r);

struct re_char_class_ast *
re_char_class_ast_literal(unsigned char c);

struct re_char_class_ast *
re_char_class_ast_range(const struct ast_range_endpoint *from, struct ast_pos start,
    const struct ast_range_endpoint *to, struct ast_pos end);

void
re_char_class_endpoint_span(const struct ast_range_endpoint *r,
    unsigned char *from, unsigned char *to);

struct re_char_class_ast *
re_char_class_ast_flags(enum re_char_class_flags flags);

struct re_char_class_ast *
re_char_class_ast_named_class(char_class_constructor_fun *ctor);

struct re_char_class_ast *
re_char_class_ast_subtract(struct re_char_class_ast *ast,
    struct re_char_class_ast *mask);

void
re_char_class_ast_free(struct re_char_class_ast *ast);

int
re_char_class_ast_compile(struct re_char_class_ast *cca,
    struct fsm *fsm, enum re_flags flags,
    struct re_err *err, const struct fsm_options *opt,
    struct fsm_state *x, struct fsm_state *y);

const char *
re_char_class_id_str(enum ast_char_class_id id);


/* Dialect-specific char class / char type handling.
 * These are defined in src/libre/dialect/${dialect}/re_dialect_${dialect}.c */
enum re_dialect_char_class_lookup_res {
	/* Unknown class, a syntax error */
	RE_CLASS_NOT_FOUND,
	/* Found and supported */
	RE_CLASS_FOUND,
	/* Recognized but explicitly not supported */
	RE_CLASS_UNSUPPORTED = -1
};

typedef enum re_dialect_char_class_lookup_res
re_dialect_char_class_lookup(const char *name, char_class_constructor_fun **res);

re_dialect_char_class_lookup re_char_class_literal;
re_dialect_char_class_lookup re_char_class_like;
re_dialect_char_class_lookup re_char_class_glob;
re_dialect_char_class_lookup re_char_class_sql;
re_dialect_char_class_lookup re_char_class_native;
re_dialect_char_class_lookup re_char_class_pcre;

#endif
