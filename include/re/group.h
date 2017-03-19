/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef RE_GROUP_H
#define RE_GROUP_H

struct re_err;

int
re_group_print(enum re_dialect dialect, int (*getc)(void *opaque), void *opaque,
	enum re_flags flags, int overlap,
	struct re_err *err,
	FILE *f,
	int boxed,
	int (*escputc)(int c, FILE *f));

int
re_group_snprint(enum re_dialect dialect, int (*getc)(void *opaque), void *opaque,
	enum re_flags flags, int overlap,
	struct re_err *err,
	char *s, size_t n,
	int boxed,
	int (*escputc)(int c, FILE *f));

#endif

