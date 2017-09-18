/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef XALLOC_H
#define XALLOC_H

/*
 * Duplicate a string. Returns NULL on error.
 */
char *
xstrdup(const char *s);

/*
 * The following functions are for CLI use only; they exit on error
 */

void *
xmalloc(size_t sz);

void *
xcalloc(size_t count, size_t sz);

void *
xrealloc(void *p, size_t sz);

#endif

