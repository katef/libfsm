/* $Id$ */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <re/re.h>

void
re_perror(const char *func, enum re_form form, const struct re_err *err,
	const char *file, const char *s)
{
	assert(func != NULL);
	assert(err != NULL);

	if (err->e & RE_SYNTAX) {
		fprintf(stderr, "%s: %s\n", func, re_strerror(err->e));
		return;
	}

	if (file != NULL) {
		fprintf(stderr, "%s:", file);
	}

	fprintf(stderr, "%u", err->byte + 1);

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

	fprintf(stderr, ": %s\n", re_strerror(err->e));
}

