/* $Id$ */

#include <assert.h>
#include <limits.h>
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

	if (s != NULL) {
		char delim;

		if (file != NULL) {
			fprintf(stderr, ": ");
		}

		switch (form) {
		case RE_LITERAL: delim = '\''; break;
		case RE_GLOB:    delim = '\"'; break;
		default:         delim = '/';  break;
		}

		fprintf(stderr, ": %c%s%c", delim, s, delim);
	}

	if (err->e & RE_SYNTAX) {
		if (file != NULL || s != NULL) {
			fprintf(stderr, ":");
		}

		fprintf(stderr, "%u", err->byte + 1);
	}

	if (func != NULL) {
		if (file != NULL || s != NULL || err->e & RE_SYNTAX) {
			fprintf(stderr, ": ");
		}

		fprintf(stderr, "%s", func);
	}

	switch (err->e) {
	case RE_EHEXRANGE:
		fprintf(stderr, ": Hex escape %s out of range: expected \\0..\\x%X inclusive",
			err->esc, UCHAR_MAX);
		break;

	case RE_EOCTRANGE:
		fprintf(stderr, ": Octal escape %s out of range: expected \\0..\\%o inclusive",
			err->esc, UCHAR_MAX);
		break;

	default:
		fprintf(stderr, ": %s", re_strerror(err->e));
		break;
	}

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

