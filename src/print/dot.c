/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>

#include <fsm/fsm.h>
#include <fsm/options.h>

#include <print/esc.h>

int
dot_escputc_html(FILE *f, const struct fsm_options *opt, char c)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (opt->always_hex) {
		return fprintf(f, "\\x%02x", (unsigned char) c); /* for humans */
	}

	switch (c) {
	case '&':  return fputs("&amp;",  f);
	case '\"': return fputs("&quot;", f);
	case ']':  return fputs("&#x5D;", f); /* yes, even in a HTML label */
	case '<':  return fputs("&#x3C;", f);
	case '>':  return fputs("&#x3E;", f);
	case ' ':  return fputs("&#x2423;", f);

	/* for humans */
	case '\a': return fputs("\\a", f);
	case '\b': return fputs("\\b", f);
	case '\f': return fputs("\\f", f);
	case '\n': return fputs("\\n", f);
	case '\r': return fputs("\\r", f);
	case '\t': return fputs("\\t", f);
	case '\v': return fputs("\\v", f);

	default:
		break;
	}

	if (!isprint((unsigned char) c)) {
		return fprintf(f, "\\x%02x", (unsigned char) c); /* for humans */
	}

	return fprintf(f, "%c", c);
}

