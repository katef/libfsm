/* $Id$ */

#include <assert.h>
#include <stdio.h>

#include <re/re.h>

int
re_getc_str(void *opaque)
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
re_getc_file(void *opaque)
{
	FILE *f;

	f = opaque;

	assert(f  != NULL);

	return fgetc(f);
}

