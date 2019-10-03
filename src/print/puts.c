/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <ctype.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include <fsm/fsm.h>
#include <fsm/options.h>

#include <print/esc.h>

int
escputs(FILE *f, const struct fsm_options *opt, escputc *escputc,
	const char *s)
{
	const char *p;
	int r, n;

	assert(f != NULL);
	assert(opt != NULL);
	assert(escputc != NULL);
	assert(s != NULL);

	n = 0;

	for (p = s; *p != '\0'; p++) {
		r = escputc(f, opt, *p);
		if (r == -1) {
			return -1;
		}

		n += r;
	}

	return n;
}

