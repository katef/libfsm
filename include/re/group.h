/* $Id$ */

#ifndef RE_GROUP_H
#define RE_GROUP_H

struct re_err;

int
re_group_print(enum re_form form, int (*getc)(void *opaque), void *opaque,
	enum re_flags flags, struct re_err *err,
	FILE *f,
	int (*escputc)(int c, FILE *f));

int
re_group_snprint(enum re_form form, int (*getc)(void *opaque), void *opaque,
    enum re_flags flags, struct re_err *err,
    char *s, size_t n,
    int (*escputc)(int c, FILE *f));

#endif

