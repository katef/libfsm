/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <re/re.h>

static int
re_fprint(FILE *f, enum re_dialect dialect, const char *s)
{
	char start, end;

	assert(f != NULL);
	assert(s != NULL);

	if (dialect & RE_GROUP) {
		start = '[';
		end   = ']';
	} else {
		dialect &= ~RE_GROUP;

		switch (dialect) {
		case RE_LITERAL: start = '\''; end = start; break;
		case RE_GLOB:    start = '\"'; end = start; break;
		default:         start = '/';  end = start; break;
		}
	}

	/* TODO: escape per surrounding delim */
	return fprintf(f, "%c%s%c", start, s, end);
}

void
re_perror(enum re_dialect dialect, const struct re_err *err,
	const char *file, const char *s)
{
	assert(err != NULL);

	if (file != NULL) {
		fprintf(stderr, "%s", file);
	}

	if (s != NULL) {
		if (file != NULL) {
			fprintf(stderr, ": ");
		}

		re_fprint(stderr, dialect, s);
	}

	if (err->e & RE_MARK) {
		assert(err->end.byte >= err->start.byte);

		if (file != NULL || s != NULL) {
			fprintf(stderr, ":");
		}

		if (err->end.byte == err->start.byte) {
			fprintf(stderr, "%u",    err->start.byte + 1);
		} else {
			fprintf(stderr, "%u-%u", err->start.byte + 1, err->end.byte + 1);
		}
	}

	switch (err->e) {
	case RE_EHEXRANGE:   fprintf(stderr, ": Hex escape %s out of range",   err->esc); break;
	case RE_EOCTRANGE:   fprintf(stderr, ": Octal escape %s out of range", err->esc); break;
	case RE_ECOUNTRANGE: fprintf(stderr, ": Count %s out of range",        err->esc); break;

	default:
		fprintf(stderr, ": %s", re_strerror(err->e));
		break;
	}

	switch (err->e) {
	case RE_EHEXRANGE:   fprintf(stderr, ": expected \\0..\\x%X inclusive", UCHAR_MAX); break;
	case RE_EOCTRANGE:   fprintf(stderr, ": expected \\0..\\%o inclusive",  UCHAR_MAX); break;
	case RE_ECOUNTRANGE: fprintf(stderr, ": expected 1..%u inclusive",      UINT_MAX);  break;
	case RE_EXESC:       fprintf(stderr, " \\0..\\x%X inclusive",           UCHAR_MAX); break;
	case RE_EXCOUNT:     fprintf(stderr, " 1..%u inclusive",                UINT_MAX);  break;
	case RE_ENEGCOUNT:   fprintf(stderr, " {%u,%u}",                   err->m, err->n); break;
	case RE_EBADCP:      fprintf(stderr, ": U+%06lX",                         err->cp); break;

	default:
		;
	}

	/* TODO: escape */
	switch (err->e) {
	case RE_ENEGRANGE:
		fprintf(stderr, " [%s]", err->set);
		break;

	default:
		;
	}

	fprintf(stderr, "\n");
}

