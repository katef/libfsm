/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <ctype.h>
#include <assert.h>
#include <stdio.h>

#include <fsm/options.h>

#include <print/esc.h>

#include "libfsm/internal.h" /* XXX */

int
json_escputc(FILE *f, const struct fsm_options *opt, int c)
{
	size_t i;

	const struct {
		int c;
		const char *s;
	} a[] = {
		{ '\\', "\\\\" },
		{ '\"', "\\\"" },
		{ '/',  "\\/"  },

		{ '\b', "\\b"  },
		{ '\f', "\\f"  },
		{ '\n', "\\n"  },
		{ '\r', "\\r"  },
		{ '\t', "\\t"  }
	};

	assert(f != NULL);
	assert(opt != NULL);
	assert(c != FSM_EDGE_EPSILON);

	if (opt->always_hex) {
		return fprintf(f, "\\u%04x", (unsigned char) c);
	}

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (a[i].c == c) {
			return fputs(a[i].s, f);
		}
	}

	if (!isprint((unsigned char) c)) {
		return fprintf(f, "\\u%04x", (unsigned char) c);
	}

	return fprintf(f, "%c", c);
}

