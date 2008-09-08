/* $Id$ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "xmalloc.h"

void *xmalloc(size_t size) {
	void *new;

	new = malloc(size);
	if (new == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	return new;
}

char *xstrdup(const char *s) {
	char *new;

	new = malloc(strlen(s) + 1);
	if (new == NULL) {
		return NULL;
	}

	return strcpy(new, s);
}

