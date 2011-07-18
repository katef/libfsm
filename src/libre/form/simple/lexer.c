/* $Id$ */

/*
 * TODO: \escapes
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "lexer.h"

enum state {
	STATE_LITERAL,
	STATE_GROUP,
	STATE_COUNT,
	STATE_COUNTEND
};

/* TODO: centralise all lexers' states? */
struct lex_state {
	int (*f)(void *opaque);
	void *opaque;
	char c;
	unsigned u;
	enum state state;
};

struct lex_state *
lex_simple_init(int (*f)(void *opaque), void *opaque)
{
	struct lex_state *new;

	assert(f != NULL);

	new = malloc(sizeof *new);
	if (new == NULL) {
		return NULL;
	}

	new->f      = f;
	new->opaque = opaque;
	new->state  = STATE_LITERAL;

	return new;
}

void
lex_simple_free(struct lex_state *state)
{
	assert(state != NULL);

	free(state);
}

static enum lex_tok
nexttoken_esc(struct lex_state *state)
{
	int c;

	assert(state != NULL);

	c = state->f(state->opaque);

	switch (c) {
	case '\\': state->c = '\\'; break;
	case 't':  state->c = '\t'; break;
	case 'n':  state->c = '\n'; break;
	case 'r':  state->c = '\r'; break;
	case 'v':  state->c = '\v'; break;
	case 'f':  state->c = '\f'; break;
	default:   state->c = c;    break;
	}

	switch (c) {
	case EOF: return TOK_EOF;
	default:  return TOK_CHAR;
	}
}

static enum lex_tok
nexttoken_literal(struct lex_state *state)
{
	int c;

	assert(state != NULL);

	c = state->f(state->opaque);

	state->c = c;

	switch (c) {
	case EOF:                              return TOK_EOF;
	case '^':                              return TOK_SOL;
	case '$':                              return TOK_EOL;
	case '?':                              return TOK_QMARK;
	case '*':                              return TOK_STAR;
	case '+':                              return TOK_PLUS;
	case '.':                              return TOK_DOT;
	case '|':                              return TOK_ALT;
	case '(':                              return TOK_OPEN__SUB;
	case ')':                              return TOK_CLOSE__SUB;
	case '\\':                             return nexttoken_esc(state);
	case '[':  state->state = STATE_GROUP; return TOK_OPEN__GROUP;
	case '{':  state->state = STATE_COUNT; return TOK_OPEN__COUNT;
	default:                               return TOK_CHAR;
	}
}

static enum lex_tok
nexttoken_group(struct lex_state *state)
{
	int c;

	assert(state != NULL);

	c = state->f(state->opaque);

	state->c = c;

	switch (c) {
	case EOF:                                return TOK_EOF;
	case '^':                                return TOK_SOL;	/* TODO: rename */
	case '-':                                return TOK_RANGESEP;
	case '\\':                               return nexttoken_esc(state);
	case ']':  state->state = STATE_LITERAL; return TOK_CLOSE__GROUP;
	default:                                 return TOK_CHAR;
	}
}

static enum lex_tok
nexttoken_count(struct lex_state *state)
{
	int c;

	assert(state != NULL);

	c = state->f(state->opaque);

	state->c = c;

	if (isdigit(c)) {
		state->u *= 10;
		state->u += c - '0';

		return nexttoken_count(state);
	}

	switch (c) {
	case EOF:                                return TOK_EOF;
	case ',': state->state = STATE_COUNTEND; return TOK_COUNT;
	case '}': state->state = STATE_COUNTEND; return TOK_COUNT;
	default:                                 return TOK_CHAR;
	}
}

static enum lex_tok
nexttoken_countend(struct lex_state *state)
{
	assert(state != NULL);

	assert(state->c == '}' || state->c == ',');

	switch (state->c) {
	case ',': state->state = STATE_COUNT;   return TOK_COUNTSEP;
	case '}': state->state = STATE_LITERAL; return TOK_CLOSE__COUNT;
	}
}

enum lex_tok
lex_simple_nexttoken(struct lex_state *state)
{
	assert(state != NULL);
	assert(state->f != NULL);

	state->u = 0U;

	switch (state->state) {
	case STATE_LITERAL:  return nexttoken_literal(state);
	case STATE_GROUP:    return nexttoken_group(state);
	case STATE_COUNT:    return nexttoken_count(state);
	case STATE_COUNTEND: return nexttoken_countend(state);
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

unsigned
lex_simple_tokval_u(struct lex_state *state)
{
	assert(state != NULL);

	return state->u;
}

