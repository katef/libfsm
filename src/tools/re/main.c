/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fsm/graph.h>	/* XXX */
#include <fsm/out.h>	/* XXX */
#include <re/re.h>

#include "libre/internal.h"	/* XXX */

/*
 * TODO: match a regexp against arguments. return $? if all match
 *
 * TODO: pass a language option for dumping output.
 * output to various forms, too (print the fsm as ERE, glob, etc)
 * do this through a proper interface, instead of including internal.h
 *
 * TODO: accepting a delimiter would be useful: /abc/. perhaps provide that as
 * a convenience function, especially wrt. escaping for lexing. Also convenient
 * for specifying flags: /abc/g
 *
 * getopts:
 * pass optarg for "form" of regex. -g for glob etc
 * remaining argv is string(s) to match
 *
 * ./re -e '^012.*abc$' -b '[a-z]_xx$' -g 'abc*def' "some string" "some other string"
 */

static void usage(void) {
	fprintf(stderr, "usage: re [-h] { [-id] [-lgeb9p] <re> } <string>\n");
}

static enum re_form form(char c) {
	switch (c) {
/* TODO:
	case 'e': return RE_ERE;
	case 'b': return RE_BRE;
	case '9': return RE_PLAN9;
	case 'p': return RE_PCRE;
*/
	case 'l': return RE_LITERAL;
	case 'g': return RE_GLOB;
	case 's': return RE_SIMPLE;

	default:
		fprintf(stderr, "unrecognised re form \"%c\"", c);
		exit(EXIT_FAILURE);
	}

	assert(!"unreached");
}

int main(int argc, char *argv[]) {
	struct re *re;
	int files;
	int dump;

	if (argc < 2) {
		usage();
		return EXIT_FAILURE;
	}

	re = re_new_empty();
	if (re == NULL) {
		perror("re_new_empty");
		return EXIT_FAILURE;
	}

	files = 0;
	dump  = 0;

	{
		int c;

		while ((c = getopt(argc, argv, "hdil:g:s:")) != -1) {
			struct re *new;
			enum re_err err;

			switch (c) {
			case 'h':
				usage();
				return EXIT_SUCCESS;

/* TODO:
			case 'b':
			case 'e':
			case '9':
			case 'p':
*/
			case 'd':
				dump = 1;
				break;

			case 'i':
				files = 1;
				break;

			case 'l':
			case 'g':
			case 's':
				/* TODO: flags? */

				if (files) {
					FILE *f;

					f = fopen(optarg, "r");
					if (f == NULL) {
						perror(optarg);
						return EXIT_FAILURE;
					}

					new = re_new_comp(form(c), re_getc_file, f, 0, &err);

					fclose(f);
				} else {
					new = re_new_comp(form(c), re_getc_str, &optarg, 0, &err);
				}

				if (new == NULL) {
					fprintf(stderr, "re_new_comp: %s\n", re_strerror(err));
					return EXIT_FAILURE;
				}

				/* TODO: associate optarg with new's end state */

				if (!re_union(re, new)) {
					perror("re_merge");
					return EXIT_FAILURE;
				}

				break;

			case '?':
			default:
				usage();
				return EXIT_FAILURE;
			}
		}

		argc -= optind;
		argv += optind;
	}

	if (!fsm_todfa(re->fsm)) {
		perror("fsm_todfa");
		return EXIT_FAILURE;
	}

	if (dump) {
		fsm_print(re->fsm, stdout, FSM_OUT_FSM);
	}

	/* TODO: flags? */
	re_exec(re, argv[0], 0);

	re_free(re);

	return EXIT_SUCCESS;
}

