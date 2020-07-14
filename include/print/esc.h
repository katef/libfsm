/*
 * Copyright 2018 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#ifndef PRINT_ESC_H
#define PRINT_ESC_H

struct fsm_options;

typedef int (escputc)(FILE *f, const struct fsm_options *opt, char c);

escputc c_escputc_char;
escputc c_escputc_str;
escputc abnf_escputc;
escputc dot_escputc_html;
escputc dot_escputc_html_record;
escputc fsm_escputc;
escputc json_escputc;
escputc pcre_escputc;

void
c_escputcharlit(FILE *f, const struct fsm_options *opt, char c);

int
escputs(FILE *f, const struct fsm_options *opt, escputc *escputc,
	const char *s);

void
esctok(FILE *f, const char *s);

#endif

