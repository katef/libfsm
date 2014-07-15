/* $Id$ */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include LX_HEADER

char a[256]; /* XXX: bounds check, and local by lx->tokbuf opaque */
char *p;

static int
push(struct lx *lx, char c)
{
	(void) lx;

	assert(lx != NULL);
	assert(c != EOF);

	*p++ = c;
}

static int
pop(struct lx *lx)
{
	(void) lx;

	p--;
}

int
main(void)
{
	enum lx_token t;
	struct lx lx = { 0 };

	lx.lgetc  = lx_fgetc;
	lx.opaque = stdin;

	lx.push = push;
	lx.pop  = pop;

	/* TODO: make an lx_init() for this, plus other fields */
	lx.c = EOF;

	do {

		p = a;

		t = lx_next(&lx);
		switch (t) {
		case TOK_EOF:
			printf("%u: <EOF>\n", lx.byte);
			break;

		case TOK_SKIP:
			continue;

		case TOK_ERROR:
			perror("lx_next");
			break;

		case TOK_UNKNOWN:
			fprintf(stderr, "%u: lexically uncategorised: '%.*s'\n",
				lx.byte,
				(int) (p - a), a);
			break;

		default:
			printf("%u: <%s '%.*s'>\n",
				lx.byte,
				lx_name(t),
				(int) (p - a), a);
			break;
		}

	} while (t != TOK_ERROR && t != TOK_EOF && t != TOK_UNKNOWN);

	return t == TOK_ERROR;
}

