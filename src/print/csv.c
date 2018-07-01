/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <fsm/options.h>

#include <print/esc.h>

#include "libfsm/internal.h" /* XXX */

int
csv_escputc(FILE *f, const struct fsm_options *opt, int c)
{
	size_t i;

	const struct {
		int c;
		const char *s;
	} a[] = {
		{ FSM_EDGE_EPSILON, "\xCE\xB5" }, /* epsilon, U03B5 UTF-8 */

		{ '\"', "\\\"" }
		/* TODO: others */
	};

	assert(f != NULL);
	assert(opt != NULL);

	if (opt->always_hex) {
		return fprintf(f, "0x%02X", (unsigned char) c);
	}

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (a[i].c == c) {
			return fputs(a[i].s, f);
		}
	}

	if (!isprint((unsigned char) c)) {
		return printf("0x%02X", (unsigned char) c);
	}

	return fprintf(f, "%c", c);
}

