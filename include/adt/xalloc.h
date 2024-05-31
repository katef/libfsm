/*
 * Copyright 2008-2024 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef XALLOC_H
#define XALLOC_H

/*
 * These functions are for CLI use only; they exit on error
 */

char *
xstrdup(const char *s);

char *
xstrndup(const char *s, size_t n);

void *
xmalloc(size_t sz);

void *
xcalloc(size_t count, size_t sz);

/* equivalent to free(p) when sz is 0, returns NULL */
void *
xrealloc(void *p, size_t sz);

#endif

