/* $Id: lexer.h 2 2009-03-08 17:12:37Z kate $ */

#ifndef RE_FORM_GLOB_LEXER_H
#define RE_FORM_GLOB_LEXER_H

struct lex_state;

enum lex_tok {
	TOK_QMARK,
	TOK_STAR,
	TOK_CHAR,
	TOK_EOF,
	TOK_ERROR
};

struct lex_state *
lex_glob_init(int (*getc)(void *opaque), void *opaque);

void
lex_glob_free(struct lex_state *state);

enum lex_tok
lex_glob_nexttoken(struct lex_state *state);

char
lex_glob_tokval(struct lex_state *state);

#endif

