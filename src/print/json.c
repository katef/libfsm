/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <ctype.h>
#include <assert.h>
#include <stdio.h>
#include <limits.h>

#include <fsm/fsm.h>
#include <fsm/options.h>

#include <print/esc.h>

int
json_escputc(FILE *f, const struct fsm_options *opt, char c)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (opt->always_hex) {
		return fprintf(f, "\\u%04x", (unsigned char) c);
	}

	switch (c) {
	case '\\': return fputs("\\\\", f);
	case '\"': return fputs("\\\"", f);
	case '/':  return fputs("\\/",  f);

	case '\b': return fputs("\\b", f);
	case '\f': return fputs("\\f", f);
	case '\n': return fputs("\\n", f);
	case '\r': return fputs("\\r", f);
	case '\t': return fputs("\\t", f);

	default:
		break;
	}

	if (!isprint((unsigned char) c)) {
		return fprintf(f, "\\u%04x", (unsigned char) c);
	}

	return fprintf(f, "%c", c);
}

