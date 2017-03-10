#ifndef LX_INTERNAL_OUT_H
#define LX_INTERNAL_OUT_H

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

void
lx_out_c(const struct ast *ast, FILE *f);

void
lx_out_h(const struct ast *ast, FILE *f);

void
lx_out_dot(const struct ast *ast, FILE *f);

void
lx_out_dump(const struct ast *ast, FILE *f);

void
lx_out_zdot(const struct ast *ast, FILE *f);

#endif

