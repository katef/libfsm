/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <ctype.h>
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include <fsm/fsm.h>
#include <fsm/options.h>

#include <print/esc.h>

int
pcre_escputc(FILE *f, const struct fsm_options *opt, char c)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (opt->always_hex) {
		return fprintf(f, "\\x%02x", (unsigned char) c);
	}

	switch (c) {
	case '^': return fputs("\\^", f);
	case '$': return fputs("\\$", f);
	case '(': return fputs("\\(", f);
	case ')': return fputs("\\)", f);

	case '|': return fputs("\\|", f);
	case '[': return fputs("\\[", f);
	case ']': return fputs("\\]", f);
	case '+': return fputs("\\+", f);
	case '*': return fputs("\\*", f);
	case '?': return fputs("\\?", f);
	case '.': return fputs("\\.", f);

	case '\f': return fputs("\\f", f);
	case '\n': return fputs("\\n", f);
	case '\r': return fputs("\\r", f);
	case '\t': return fputs("\\t", f);

	/* TODO: probably others; this is a cursory attempt */

	case '\\': return fputs("\\\\", f);

	default:
		break;
	}

	if (!isprint((unsigned char) c)) {
		return fprintf(f, "\\x%02x", (unsigned char) c);
	}

	return fprintf(f, "%c", c);
}

