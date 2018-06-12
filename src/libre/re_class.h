#ifndef RE_CLASS_H
#define RE_CLASS_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

enum ast_char_class_flags {
	AST_CHAR_CLASS_FLAG_NONE = 0x00,
	/* the class should be negated, e.g. [^aeiou] */
	AST_CHAR_CLASS_FLAG_INVERTED = 0x01,
	/* includes the `-` character, which isn't part of a range */
	AST_CHAR_CLASS_FLAG_MINUS = 0x02
};

struct ast_char_class {
	enum ast_char_class_flags flags;
	/* This only handles single-byte chars for now. */
	unsigned char chars[256/8];
};

struct ast_char_class *
ast_char_class_new(void);

void
ast_char_class_free(struct ast_char_class *c);

void
ast_char_class_add_byte(struct ast_char_class *c, unsigned char byte);

void
ast_char_class_add_range(struct ast_char_class *c, unsigned char from, unsigned char to);

void
ast_char_class_invert(struct ast_char_class *cc);

/* void
 * ast_char_class_add_named_class(struct ast_char_class *c, unsigned char from, unsigned char to); */

void
ast_char_class_mask(struct ast_char_class *c, const struct ast_char_class *mask);

void
ast_char_class_dump(FILE *f, struct ast_char_class *c);

#endif
