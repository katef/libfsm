/* $Id$ */

#include <stdlib.h>
#include <string.h>

#include "xalloc.h"

char *xstrdup(const char *s) {
	char *new;

	new = malloc(strlen(s) + 1);
	if (!new) {
		return NULL;
	}

	return strcpy(new, s);
}

