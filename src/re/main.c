/* $Id$ */

#define _POSIX_C_SOURCE 2

#include <unistd.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <fsm/fsm.h>
#include <fsm/out.h>	/* XXX */
#include <fsm/bool.h>
#include <fsm/pred.h>
#include <fsm/exec.h>
#include <fsm/graph.h>

#include <re/re.h>

#include "libfsm/internal.h" /* XXX */

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

static void
usage(void)
{
	fprintf(stderr, "usage: re [-h] { [-cidmn] [-lgeb9ps] <re> } <string>\n");
}

static enum re_form
form(char c)
{
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

int
main(int argc, char *argv[])
{
	struct fsm *(*join)(struct fsm *, struct fsm *);
	struct fsm *fsm;
	int files;
	int dump;
	int example;
	int keep_nfa;
	int r;

	if (argc < 2) {
		usage();
		return EXIT_FAILURE;
	}

	fsm = re_new_empty();
	if (fsm == NULL) {
		perror("re_new_empty");
		return EXIT_FAILURE;
	}

	files    = 0;
	dump     = 0;
	example  = 0;
	keep_nfa = 0;
	join     = fsm_union;

	{
		int c;

		while (c = getopt(argc, argv, "hcdil:mng:s:"), c != -1) {
			struct re_err err;
			struct fsm *new;

			switch (c) {
			case 'h':
				usage();
				return EXIT_SUCCESS;

			case 'c':
				join = fsm_concat;
				break;

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

			case 'm':
				example = 1;
				break;

			case 'n':
				keep_nfa = 1;
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

					new = re_new_comp(form(c), re_fgetc, f, 0, &err);

					fclose(f);
				} else {
					const char *s;

					s = optarg;

					new = re_new_comp(form(c), re_sgetc, &s, 0, &err);
				}

				/* TODO: addend(new, optarg); */

				if (new == NULL) {
					re_perror("re_new_comp", form(c), &err,
						 files ? optarg : NULL,
						!files ? optarg : NULL);
					return EXIT_FAILURE;
				}

				/* TODO: associate optarg with new's end state */

				fsm = join(fsm, new);
				if (fsm == NULL) {
					perror("fsm_union/concat");
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

	if (keep_nfa && argc > 0) {
		fprintf(stderr, "-n is not for execution; execution requires a DFA\n");
		return EXIT_FAILURE;
	}

	if (!keep_nfa) {
		if (!fsm_minimise(fsm)) {
			perror("fsm_minimise");
			return EXIT_FAILURE;
		}
	}

	if (example) {
		struct fsm_state *s;
		char buf[256]; /* TODO */
		int n;

		for (s = fsm->sl; s != NULL; s = s->next) {
			if (!fsm_isend(fsm, s)) {
				continue;
			}

			n = fsm_example(fsm, s, buf, sizeof buf);
			if (-1 == n) {
				perror("fsm_example");
				return EXIT_FAILURE;
			}

			/* TODO: escape hex etc */
			fprintf(stderr, "%s%s\n", buf,
				n >= (int) sizeof buf - 1 ? "..." : "");
		}
	}

	if (dump) {
		fsm_print(fsm, stdout, FSM_OUT_FSM, NULL);
	}

	r = 0;

	{
		int i;

		for (i = 0; i < argc; i++) {
			r += !fsm_exec(fsm, fsm_sgetc, &argv[i]);
		}
	}

	fsm_free(fsm);

	return r;
}

