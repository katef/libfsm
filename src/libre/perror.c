/* $Id$ */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <re/re.h>

void
re_perror(const char *func, enum re_form form, const struct re_err *err,
	const char *file, const char *s)
{
	assert(func != NULL);
	assert(err != NULL);

	if (file != NULL) {
		fprintf(stderr, "%s", file);
	}

	if (err->e & RE_SYNTAX) {
		if (file != NULL) {
			fprintf(stderr, ":");
		}
		fprintf(stderr, "%u", err->byte + 1);
	}

	if (s == NULL) {
		fprintf(stderr, ": %s", func);
	} else {
		char delim;

		switch (form) {
		case RE_LITERAL: delim = '\''; break;
		case RE_GLOB:    delim = '\"'; break;
		default:         delim = '/';  break;
		}

		fprintf(stderr, ": %c%s%c", delim, s, delim);
	}

	fprintf(stderr, ": %s", re_strerror(err->e));

	if (err->e == RE_EOVERLAP) {
		if (0 == strcmp(err->set, err->dup)) {
			fprintf(stderr, ": overlap of %s",
				err->dup);
		} else {
			fprintf(stderr, ": overlap of %s; minimal coverage is %s",
				err->dup, err->set);
		}
	}

	fprintf(stderr, "\n");
}

