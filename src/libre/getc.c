/* $Id$ */

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

int
re_bgetc(void *opaque)
{
	struct re_buf *b = opaque;

	assert(b != NULL);
	assert(b->ptr != NULL);
	assert(b->pos <= b->len);

	if (b->pos == b->len) {
		return EOF;
	}

	return (unsigned char) b->ptr[b->pos++];
}
