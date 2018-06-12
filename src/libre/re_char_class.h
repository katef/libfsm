#ifndef RE_CHAR_CLASS_H
#define RE_CHAR_CLASS_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

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
	RE_CHAR_CLASS_AST_FLAGS,
	RE_CHAR_CLASS_AST_SUBTRACT
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


/* TODO: opaque? */
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
			unsigned char from;
			unsigned char to;
		} range;
		struct {
			enum ast_class_id id;
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
re_char_class_ast_range(unsigned char from, unsigned char to);

struct re_char_class_ast *
re_char_class_ast_flags(enum re_char_class_flags flags);

struct re_char_class_ast *
re_char_class_ast_named_class(enum ast_class_id id);

struct re_char_class_ast *
re_char_class_ast_subtract(struct re_char_class_ast *ast,
    struct re_char_class_ast *mask);

void
re_char_class_ast_prettyprint(FILE *f,
    struct re_char_class_ast *ast, size_t indent);

void
re_char_class_ast_free(struct re_char_class_ast *ast);

struct re_char_class *
re_char_class_ast_compile(struct re_char_class_ast *cca);

void
ast_char_class_dump(FILE *f, struct re_char_class *c);

#endif
