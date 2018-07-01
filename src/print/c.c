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

#include <fsm/options.h>

#include <print/esc.h>

#include "libfsm/internal.h" /* XXX */

int
c_escputc_char(FILE *f, const struct fsm_options *opt, int c)
{
	size_t i;

	const struct {
		int c;
		const char *s;
	} a[] = {
		{ '\\', "\\\\" },
		{ '\'', "\\\'" },

		{ '\a', "\\a"  },
		{ '\b', "\\b"  },
		{ '\f', "\\f"  },
		{ '\n', "\\n"  },
		{ '\r', "\\r"  },
		{ '\t', "\\t"  },
		{ '\v', "\\v"  }
	};

	assert(f != NULL);
	assert(opt != NULL);
	assert(c != FSM_EDGE_EPSILON);

	if (opt->always_hex) {
		return fprintf(f, "\\x%02x", (unsigned char) c);
	}

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (a[i].c == c) {
			return fputs(a[i].s, f);
		}
	}

	if (!isprint((unsigned char) c)) {
		return fprintf(f, "\\%02x", (unsigned char) c);
	}

	return fprintf(f, "%c", c);
}

int
c_escputc_str(FILE *f, const struct fsm_options *opt, int c)
{
	size_t i;

	const struct {
		int c;
		const char *s;
	} a[] = {
		{ '\\', "\\\\" },
		{ '\"', "\\\"" },

		{ '\a', "\\a"  },
		{ '\b', "\\b"  },
		{ '\f', "\\f"  },
		{ '\n', "\\n"  },
		{ '\r', "\\r"  },
		{ '\t', "\\t"  },
		{ '\v', "\\v"  }
	};

	assert(f != NULL);
	assert(opt != NULL);
	assert(c != FSM_EDGE_EPSILON);

	if (opt->always_hex) {
		return fprintf(f, "\\x%02x", (unsigned char) c);
	}

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (a[i].c == c) {
			return fputs(a[i].s, f);
		}
	}

	/*
	 * Escaping '/' here is a lazy way to avoid keeping state when
	 * emitting '*', '/', since this is used to output example strings
	 * inside comments.
	 */

	if (!isprint((unsigned char) c) || c == '/') {
		return fprintf(f, "\\x%02x", (unsigned char) c);
	}

	return fprintf(f, "%c", c);
}

void
c_escputcharlit(FILE *f, const struct fsm_options *opt, int c)
{
	assert(f != NULL);
	assert(opt != NULL);

	if (opt->always_hex || c > SCHAR_MAX) {
		fprintf(f, "0x%02x", (unsigned char) c);
		return;
	}

	fprintf(f, "'");
	c_escputc_char(f, opt, c);
	fprintf(f, "'");
}

