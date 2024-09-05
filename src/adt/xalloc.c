/*
 * Copyright 2008-2024 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <adt/xalloc.h>

char *
xstrdup(const char *s)
{
	char *new;

	assert(s != NULL);

	new = strdup(s);
	if (new == NULL) {
		perror("strdup");
		return NULL;
	}

	return new;
}

char *
xstrndup(const char *s, size_t n)
{
	char *new;

	assert(s != NULL);

	new = strndup(s, n);
	if (new == NULL) {
		perror("strndup");
		return NULL;
	}

	return new;
}

void *
xmalloc(size_t sz)
{
	void *p;

	p = malloc(sz);
	if (p == NULL) {
		perror("malloc");
		abort();
	}

	return p;
}

void *
xcalloc(size_t count, size_t sz)
{
	void *p;

	p = calloc(count, sz);
	if (p == NULL) {
		perror("calloc");
		abort();
	}

	return p;
}

void *
xrealloc(void *p, size_t sz)
{
	void *q;

	q = realloc(p, sz);
	if (sz > 0 && q == NULL) {
		perror("realloc");
		abort();
	}

	return q;
}

