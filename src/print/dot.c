/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>

#include <fsm/options.h>

#include <print/esc.h>

#include "libfsm/internal.h" /* XXX */

int
dot_escputc_html(FILE *f, const struct fsm_options *opt, int c)
{
	size_t i;

	const struct {
		int c;
		const char *s;
	} a[] = {
		{ FSM_EDGE_EPSILON, "&#x3B5;" },

		{ '&',  "&amp;"    },
		{ '\"', "&quot;"   },
		{ ']',  "&#x5D;"   }, /* yes, even in a HTML label */
		{ '<',  "&#x3C;"   },
		{ '>',  "&#x3E;"   },
		{ ' ',  "&#x2423;" }
	};

	assert(f != NULL);
	assert(opt != NULL);

	if (opt->always_hex) {
		return fprintf(f, "\\x%02x", (unsigned char) c);
	}

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (a[i].c == c) {
			return fputs(a[i].s, f);
		}
	}

	if (!isprint((unsigned char) c)) {
		return fprintf(f, "\\x%02x", (unsigned char) c); /* for humans */
	}

	return fprintf(f, "%c", c);
}

