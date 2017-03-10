#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "lexer.h"

int stack[20];
unsigned int n;


static int
pop(void)
{
	if (n == 0) {
		return 0;
	}

	return stack[--n];
}

static void
push(int i)
{
	if (n == sizeof stack / sizeof *stack) {
		fprintf(stderr, "stack overflow\n");
		exit(1);
	}

	stack[n++] = i;
}

static int
lx_getc(void *opaque)
{
	assert(opaque != NULL);

	return fgetc(opaque);
}

static void
lx_ungetc(int c, void *opaque)
{
	assert(opaque != NULL);
	assert(c >= 0);

	if (EOF == ungetc(c, opaque)) {
		perror("ungetc");
		exit(1);
	}
}

int
main(void)
{
	struct lx lx = { 0 };

	lx.getc   = lx_getc;
	lx.ungetc = lx_ungetc;
	lx.opaque = stdin;

	for (;;) {
		switch (lx_nexttoken(&lx)) {
		case TOK_PRINT: printf("%d\n", stack[n]); continue;
		case TOK_EOF:   printf("%d\n", stack[n]); return 0;
		case TOK_ERROR: fprintf(stderr, "unexpected input\n"); return 1;

		case TOK_NUMBER: push(57); continue;	/* TODO: get spelling */

		case TOK_ADD: push(pop() + pop()); continue;
		case TOK_SUB: push(pop() - pop()); continue;
		case TOK_DIV: push(pop() / pop()); continue;
		case TOK_MUL: push(pop() * pop()); continue;
		}
	}

	printf("%d\n", pop());

	return 0;
}

