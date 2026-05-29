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
llvm_escputc_char(FILE *f, const struct fsm_options *opt, char c)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (opt->always_hex) {
		return fprintf(f, "u%#02x", (unsigned char) c);
	}

	return fprintf(f, "%u", (unsigned char) c);
}

void
llvm_escputcharlit(FILE *f, const struct fsm_options *opt, char c)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (opt->always_hex || (unsigned char) c > SCHAR_MAX) {
		fprintf(f, "u%#02x", (unsigned char) c);
		return;
	}

	llvm_escputc_char(f, opt, c);
}

