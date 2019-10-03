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
abnf_escputc(FILE *f, const struct fsm_options *opt, char c)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (opt->always_hex) {
		return fprintf(f, "%%x%02X", (unsigned char) c);
	}

	switch (c) {
	case '\"': return fputs("%x22", f);

	case '\f': return fputs("%x0C", f);
	case '\n': return fputs("%x0A", f);
	case '\r': return fputs("%x0D", f);
	case '\t': return fputs("%x09", f);
	case '\v': return fputs("%x0B", f);

	default:
		break;
	}

	if (!isprint((unsigned char) c)) {
		return fprintf(f, "%%x%02X", (unsigned char) c);
	}

	if (isalpha((unsigned char) c)) {
		return fprintf(f, "%%s\"%c\"", c);
	}

	return fprintf(f, "\"%c\"", c);
}

