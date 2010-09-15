/* $Id$ */

#ifndef RE_FORM_LITERAL_LEXER_H
#define RE_FORM_LITERAL_LEXER_H

struct lex_state;

enum lex_tok {
	TOK_CHAR,
	TOK_EOF,
	TOK_ERROR
};

struct lex_state *
lex_literal_init(int (*f)(void *opaque), void *opaque);

void
lex_literal_free(struct lex_state *state);

enum lex_tok
lex_literal_nexttoken(struct lex_state *state);

char
lex_literal_tokval(struct lex_state *state);

#endif

