/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <fsm/fsm.h>

#include <adt/set.h>

int
fsm_sgetc(void *opaque)
{
	const char **s = opaque;
	char c;

	assert(s != NULL);
	assert(*s != NULL);

	c = **s;

	if (c == '\0') {
		return EOF;
	}

	(*s)++;

	return (unsigned char) c;
}

int
fsm_fgetc(void *opaque)
{
	FILE *f = opaque;
	int c;

	assert(f != NULL);

	c = fgetc(f);
	if (c == EOF) {
		/* TODO: distinguish feof from ferror */
		return EOF;
	}

	return c;
}

