/*
 * Copyright 2008-2024 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <adt/xalloc.h>

char *
xstrdup(const char *s)
{
	char *new;

	new = malloc(strlen(s) + 1);
	if (new == NULL) {
		return NULL;
	}

	return strcpy(new, s);
}

char *
xstrndup(const char *s, size_t n)
{
	char *new;

	new = malloc(n);
	if (new == NULL) {
		return NULL;
	}

	return memcpy(new, s, n);
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

