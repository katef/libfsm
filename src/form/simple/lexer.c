/* $Id$ */

/*
 * TODO: {counts} (produce unsigned long)
 * TODO: \escapes
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"

/* TODO: centralise all lexers' states? */
struct lex_state {
	int (*getc)(void *opaque);
	void *opaque;
	char c;
};

struct lex_state *
lex_simple_init(int (*getc)(void *opaque), void *opaque)
{
	struct lex_state *new;

	assert(getc != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->getc   = getc;
	new->opaque = opaque;

	return new;
}

void
lex_simple_free(struct lex_state *state)
{
	assert(state != NULL);

	free(state);
}

enum lex_tok
lex_simple_nexttoken(struct lex_state *state)
{
	int c;

	assert(state != NULL);
	assert(state->getc != NULL);

	c = state->getc(state->opaque);

	state->c = c;

	switch (state->c) {
	case EOF: return TOK_EOF;
	case '^': return TOK_SOL;
	case '$': return TOK_EOL;
	case '?': return TOK_QMARK;
	case '*': return TOK_STAR;
	case '+': return TOK_PLUS;
	case '.': return TOK_DOT;
	case '|': return TOK_ALT;
	case '-': return TOK_SEP;
	case '(': return TOK_OPEN__SUB;   case ')': return TOK_CLOSE__SUB;
	case '[': return TOK_OPEN__GROUP; case ']': return TOK_CLOSE__GROUP;
	default:  return TOK_CHAR;
	}

	assert(!"unreached");
	return TOK_EOF;
}

char
lex_simple_tokval(struct lex_state *state)
{
	assert(state != NULL);

	return state->c;
}

