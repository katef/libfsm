/*
 * Copyright 2018 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef PRINT_ESC_H
#define PRINT_ESC_H

struct fsm_options;

typedef int (escputc)(FILE *f, const struct fsm_options *opt, char c);

escputc awk_escputc;
escputc c_escputc_char;
escputc c_escputc_str;
escputc abnf_escputc;
escputc dot_escputc_html;
escputc dot_escputc_html_record;
escputc fsm_escputc;
escputc json_escputc;
escputc pcre_escputc;
escputc rust_escputc_char;
escputc rust_escputc_str;
escputc llvm_escputc_char;

int
awk_escputcharlit(FILE *f, const struct fsm_options *opt, char c);

int
c_escputcharlit(FILE *f, const struct fsm_options *opt, char c);

void
llvm_escputcharlit(FILE *f, const struct fsm_options *opt, char c);

void
rust_escputcharlit(FILE *f, const struct fsm_options *opt, char c);

int
escputs(FILE *f, const struct fsm_options *opt, escputc *escputc,
	const char *s);

int
escputbuf(FILE *f, const struct fsm_options *opt, escputc *escputc,
	const char *buf, size_t len);

void
esctok(FILE *f, const char *s);

#endif

