/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <stdio.h>

#include <re/re.h>

int
re_sgetc(void *opaque)
{
	char **s;
	char c;

	s = opaque;

	assert(s  != NULL);
	assert(*s != NULL);

	c = **s;

	if (c == '\0') {
		return EOF;
	}

	(*s)++;

	return (unsigned char) c;
}

int
re_fgetc(void *opaque)
{
	FILE *f;

	f = opaque;

	assert(f  != NULL);

	return fgetc(f);
}

