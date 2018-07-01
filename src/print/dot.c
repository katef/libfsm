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
	assert(f != NULL);
	assert(opt != NULL);
	assert(c >= 0);

	if (c == FSM_EDGE_EPSILON) {
		return fputs("&#x3B5;", f);
	}

	assert(c <= UCHAR_MAX);

	if (opt->always_hex) {
		return fprintf(f, "\\x%02x", (unsigned char) c);
	}

	switch (c) {
	case '&':  return fputs("&amp;",  f);
	case '\"': return fputs("&quot;", f);
	case ']':  return fputs("&#x5D;", f); /* yes, even in a HTML label */
	case '<':  return fputs("&#x3C;", f);
	case '>':  return fputs("&#x3E;", f);
	case ' ':  return fputs("&#x2423;", f);

	default:
		break;
	}

	if (!isprint((unsigned char) c)) {
		return fprintf(f, "\\x%02x", (unsigned char) c); /* for humans */
	}

	return fprintf(f, "%c", c);
}

