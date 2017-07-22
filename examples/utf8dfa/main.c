/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/out.h>

static int
utf8(int cp, char c[])
{
	if (cp <= 0x7f) {
		c[0] =  cp;
		return 1;
	}

	if (cp <= 0x7ff) {
		c[0] = (cp >>  6) + 192;
		c[1] = (cp  & 63) + 128;
		return 2;
	}

	if (0xd800 <= cp && cp <= 0xdfff) {
		/* invalid */
		goto error;
	}

	if (cp <= 0xffff) {
		c[0] =  (cp >> 12) + 224;
		c[1] = ((cp >>  6) &  63) + 128;
		c[2] =  (cp  & 63) + 128;
		return 3;
	}

	if (cp <= 0x10ffff) {
		c[0] =  (cp >> 18) + 240;
		c[1] = ((cp >> 12) &  63) + 128;
		c[2] = ((cp >>  6) &  63) + 128;
		c[3] =  (cp  & 63) + 128;
		return 4;
	}

error:

	return 0;
}

static void
codepoint(struct fsm *fsm, int cp)
{
	struct fsm_state *x, *y;
	char c[4];
	int r, i;

	r = utf8(cp, c);
	if (!r) {
		fprintf(stderr, "codepoint out of range: U+%06X", (unsigned int) cp);
		exit(1);
	}

	x = fsm_getstart(fsm);
	assert(x != NULL);

	/* TODO: construct trie, assuming increasing order */

	for (i = 0; i < r; i++) {
		y = fsm_addstate(fsm);
		if (y == NULL) {
			perror("fsm_addstate");
			exit(1);
		}

		if (!fsm_addedge_literal(fsm, x, y, c[i])) {
			perror("fsm_addedge_literal");
			exit(1);
		}

		x = y;
	}

	fsm_setend(fsm, x, 1);
}

int main(void) {
	struct fsm *fsm;
	struct fsm_state *start;

	fsm = fsm_new(NULL);
	if (fsm == NULL) {
		perror("fsm_new");
		exit(1);
	}

	start = fsm_addstate(fsm);
	if (start == NULL) {
		perror("fsm_addstate");
		exit(1);
	}

	fsm_setstart(fsm, start);

	for (;;) {
		char buf[8];
		buf[sizeof buf - 1] = 'x';
		char *e;
		long l;

		if (!fgets(buf, sizeof buf, stdin)) {
			break;
		}

		if (buf[sizeof buf - 1] == '\0' && buf[sizeof buf - 2] != '\n') {
			fprintf(stderr, "overflow\n");
			exit(1);
		}

		errno = 0;
		l = strtol(buf, &e, 16);
		if (*e != '\n' || ((l == LONG_MAX || l == LONG_MIN) && errno != 0)) {
			fprintf(stderr, "parse error: %s", buf);
			exit(1);
		}

		if (l < 0 || l > INT_MAX) {
			fprintf(stderr, "hex out of range: %s", buf);
			exit(1);
		}

		/* XXX: skip surrogates */
		if (0xd800 <= l && l <= 0xdfff) {
			continue;
		}

		codepoint(fsm, l);
	}

	if (!fsm_minimise(fsm)) {
		perror("fsm_minimise");
		exit(1);
	}

	/* TODO: set prefix, output language, etc */

	fsm_print(fsm, stdout, FSM_OUT_API);

	fsm_free(fsm);

	return 0;
}

