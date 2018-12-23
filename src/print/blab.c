/*
 * Copyright 2008-2018 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <ctype.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include <fsm/options.h>

#include <print/esc.h>

int
blab_escputc(FILE *f, const struct fsm_options *opt, char c)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (opt->always_hex) {
		return fprintf(f, "\\x%02x", (unsigned char) c);
	}

	switch (c) {
	case '\"': return fputs("\\\"", f);
	case '\\': return fputs("\\\\", f);
	case '\n': return fputs("\\n",  f);
	case '\r': return fputs("\\r",  f);
	case '\t': return fputs("\\t",  f);
	case '\'': return fputs("\\\'", f);

	default:
		break;
	}

	if (!isprint((unsigned char) c)) {
		return fprintf(f, "\\x%02x", (unsigned char) c);
	}

	return fprintf(f, "%c", c);
}

