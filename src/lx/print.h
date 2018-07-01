/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef LX_INTERNAL_PRINT_H
#define LX_INTERNAL_PRINT_H

#include <stdio.h>

struct ast;

/* TODO: combine all variables here into an lx_outoptions struct */
struct prefix {
	const char *api;
	const char *tok;
	const char *lx;
};

extern struct prefix prefix;
extern int print_progress;
extern int important(unsigned int n);

enum api_tokbuf {
	API_DYNBUF   = 1 << 0,
	API_FIXEDBUF = 1 << 1
};

enum api_getc {
	API_FGETC  = 1 << 0,
	API_SGETC  = 1 << 1,
	API_AGETC  = 1 << 2,
	API_FDGETC = 1 << 3
};

enum api_exclude {
	API_NAME     = 1 << 0,
	API_EXAMPLE  = 1 << 1,
	API_COMMENTS = 1 << 2,
	API_POS      = 1 << 3,
	API_BUF      = 1 << 4
};

extern enum api_tokbuf  api_tokbuf;
extern enum api_getc    api_getc;
extern enum api_exclude api_exclude;

extern struct fsm_options opt;

typedef void (lx_print)(FILE *f, const struct ast *ast);

lx_print lx_print_c;
lx_print lx_print_h;
lx_print lx_print_dot;
lx_print lx_print_dump;
lx_print lx_print_zdot;

#endif

