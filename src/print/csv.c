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
	assert(f != NULL);
	assert(opt != NULL);
	assert(c >= 0);

	if (c == FSM_EDGE_EPSILON) {
		return fputs("\xCE\xB5", f); /* epsilon, U03B5 UTF-8 */
	}

	assert(c <= UCHAR_MAX);

	if (opt->always_hex) {
		return fprintf(f, "0x%02X", (unsigned char) c);
	}

	switch (c) {
	case '\"': return fputs("\\\"", f);
	/* TODO: others */

	default:
		break;
	}

	if (!isprint((unsigned char) c)) {
		return printf("0x%02X", (unsigned char) c);
	}

	return fprintf(f, "%c", c);
}

