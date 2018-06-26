#ifndef RE_CHAR_CLASS_H
#define RE_CHAR_CLASS_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

/* This is a duplicate of struct lx_pos, but since we're linking to
 * code with several distinct lexers, there isn't a clear lexer.h
 * to include here. The parser sees both definitions, and will
 * build a `struct ast_pos` in the appropriate places. */
struct ast_pos {
	unsigned byte;
	unsigned line;
	unsigned col;
};

enum re_char_class_flags {
	RE_CHAR_CLASS_FLAG_NONE = 0x00,
	/* the class should be negated, e.g. [^aeiou] */
	RE_CHAR_CLASS_FLAG_INVERTED = 0x01,
	/* includes the `-` character, which isn't part of a range */
	RE_CHAR_CLASS_FLAG_MINUS = 0x02
};

/* TODO: opaque? */
struct re_char_class {
	enum re_char_class_flags flags;
	/* This only handles single-byte chars for now. */
	unsigned char chars[256/8];
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

/* These are used for character type set, such as \D (non-digit)
 * or \v (vertical whitespace), which can be used in slightly different
 * ways from named character classes in PCRE. They can appear inside
 * of a character class (including in a range), or on their own. */
enum ast_char_type_id {
	/* lowercase: [dhsvw] */
	AST_CHAR_TYPE_DECIMAL,
	AST_CHAR_TYPE_HORIZ_WS,
	AST_CHAR_TYPE_WS,
	AST_CHAR_TYPE_VERT_WS,
	AST_CHAR_TYPE_WORD, 	/* [:alnum:] | '_' */
	/* uppercase: [DHSVWN] */
	AST_CHAR_TYPE_NON_DECIMAL,
	AST_CHAR_TYPE_NON_HORIZ_WS,
	AST_CHAR_TYPE_NON_WS,
	AST_CHAR_TYPE_NON_VERT_WS,
	AST_CHAR_TYPE_NON_WORD,
	/* inverse of /\n/, same as /./, unless DOTALL flag is set */
	AST_CHAR_TYPE_NON_NL
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
			enum ast_char_type_id id;
		} char_type;
		struct {
			enum ast_char_class_id id;
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
			enum ast_char_class_id id;
		} named;
		struct {
			enum ast_char_type_id id;
		} char_type;
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
re_char_class_ast_named_class(enum ast_char_class_id id);

struct re_char_class_ast *
re_char_class_ast_char_type(enum ast_char_type_id id);

struct re_char_class_ast *
re_char_class_ast_subtract(struct re_char_class_ast *ast,
    struct re_char_class_ast *mask);

void
re_char_class_ast_free(struct re_char_class_ast *ast);

struct re_char_class *
re_char_class_ast_compile(struct re_char_class_ast *cca);

/* Convert a char type (outside of a char class) into a char class.  */
struct re_char_class *
re_char_class_type_compile(enum ast_char_type_id id);

void
ast_char_class_dump(FILE *f, struct re_char_class *c);

void
re_char_class_free(struct re_char_class *cc);

const char *
re_char_class_type_id_str(enum ast_char_type_id id);

const char *
re_char_class_id_str(enum ast_char_class_id id);


/* Dialect-specific char class / char type handling.
 * These are defined in src/libre/dialect/${dialect}/re_dialect_${dialect}.c */
typedef int
re_dialect_char_class_lookup(const char *name, enum ast_char_class_id *id);
typedef int
re_dialect_char_type_lookup(const char *name, enum ast_char_type_id *id);

re_dialect_char_class_lookup re_char_class_literal;
re_dialect_char_type_lookup re_char_type_literal;

re_dialect_char_class_lookup re_char_class_like;
re_dialect_char_type_lookup re_char_type_like;

re_dialect_char_class_lookup re_char_class_glob;
re_dialect_char_type_lookup re_char_type_glob;

re_dialect_char_class_lookup re_char_class_sql;
re_dialect_char_type_lookup re_char_type_sql;

re_dialect_char_class_lookup re_char_class_native;
re_dialect_char_type_lookup re_char_type_native;

re_dialect_char_class_lookup re_char_class_pcre;
re_dialect_char_type_lookup re_char_type_pcre;

#endif
