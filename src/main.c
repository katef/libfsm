/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"

int main(void) {
	struct lex_state *lex;
	int t;

	lex = lex_init(stdin);
	if (lex == NULL) {
		perror("lex_init");
		return EXIT_FAILURE;
	}

	while ((t = lex_nexttoken(lex)) != lex_eof) {
		switch (t) {
		case lex_eof: assert(!"unreached");

		case lex_flags:
			printf("flags '%s'\n", lex_tokbuf(lex));
			break;

		case lex_pattern__regex:
			printf("regex %s\n", lex_tokbuf(lex));
			break;

		case lex_pattern__str__r:
			printf("str_r\n");
			break;

		case lex_pattern__str__t:
			printf("str_t\n");
			break;

		case lex_pattern__str__n:
			printf("str_n\n");
			break;

		case lex_pattern__literal:
			printf("literal '%s'\n", lex_tokbuf(lex));
			break;

		case lex_map:
			printf("map\n");
			break;

		case lex_to:
			printf("to\n");
			break;

		case lex_semi:
			printf("semi\n");
			break;

		case lex_alt:
			printf("alt\n");
			break;

		case lex_open:
			printf("open\n");
			break;

		case lex_close:
			printf("close\n");
			break;

		case lex_token:
			printf("token '%s'\n", lex_tokbuf(lex));
			break;

		case lex_unknown:
			printf("unknown\n");
			break;

		default:
			printf("unrecognised %d\n", t);
		}
	}

    lex_free(lex);

	return EXIT_SUCCESS;
}

