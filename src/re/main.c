/* $Id$ */

#define _POSIX_C_SOURCE 2

#include <unistd.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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
 */

static void
usage(void)
{
	fprintf(stderr, "usage: re -d [-cidmn] <re> ...\n");
	fprintf(stderr, "       re -e [-cidmn] <re> ...\n");
	fprintf(stderr, "       re    [-cidmn] <re> ... <string>\n");
	fprintf(stderr, "       re    [-cidmn] <re> ... -- <string>\n"); /* TODO: multiple strings */
	fprintf(stderr, "       re -h\n");
}

static enum re_form
form_name(const char *name)
{
	size_t i;

	struct {
		const char *name;
		enum re_form form;
	} a[] = {
/* TODO:
		{ "ere",     RE_ERE     },
		{ "bre",     RE_BRE     },
		{ "plan9",   RE_PLAN9   },
		{ "pcre",    RE_PCRE    },
*/
		{ "literal", RE_LITERAL },
		{ "glob",    RE_GLOB    },
		{ "simple",  RE_SIMPLE  }
	};

	assert(name != NULL);

	for (i = 0; i < sizeof a / sizeof *a; i++) {
		if (0 == strcmp(a[i].name, name)) {
			return a[i].form;
		}
	}

	fprintf(stderr, "unrecognised re form \"%s\"", name);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	struct fsm *(*join)(struct fsm *, struct fsm *);
	enum re_form form;
	struct fsm *fsm;
	int files;
	int dump;
	int example;
	int keep_nfa;
	int r;

	if (argc < 1) {
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
	form     = RE_SIMPLE;

	{
		int c;

		while (c = getopt(argc, argv, "hcdimnr:"), c != -1) {
			switch (c) {
			case 'h':
				usage();
				return EXIT_SUCCESS;

			case 'c':
				join = fsm_concat;
				break;

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

			case 'r':
				form = form_name(optarg);
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

	if (dump && example) {
		fprintf(stderr, "-d and -e are mutually exclusive\n");
		return EXIT_FAILURE;
	}

	{
		int i;

		for (i = 0; i < argc - 1; i++) {
			struct re_err err;
			struct fsm *new;

			/* TODO: handle possible "form:" prefix */

			if (0 == strcmp(argv[i], "--")) {
				argc--;
				argv++;

				break;
			}

			if (files) {
				FILE *f;

				f = fopen(argv[i], "r");
				if (f == NULL) {
					perror(argv[i]);
					return EXIT_FAILURE;
				}

				new = re_new_comp(form, re_fgetc, f, 0, &err);

				fclose(f);
			} else {
				const char *s;

				s = argv[i];

				new = re_new_comp(form, re_sgetc, &s, 0, &err);
			}

			/* TODO: addend(new, argv[i]); */

			if (new == NULL) {
				re_perror("re_new_comp", form, &err,
					 files ? argv[i] : NULL,
					!files ? argv[i] : NULL);
				return EXIT_FAILURE;
			}

			/* TODO: associate argv[i] with new's end state */

			fsm = join(fsm, new);
			if (fsm == NULL) {
				perror("fsm_union/concat");
				return EXIT_FAILURE;
			}
		}

		argc -= i;
		argv += i;
	}

	if ((dump || example) && argc > 0) {
		fprintf(stderr, "too many arguments\n");
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

		return 0;
	}

	if (dump) {
		fsm_print(fsm, stdout, FSM_OUT_FSM, NULL);

		return 0;
	}

	if (keep_nfa) {
		fprintf(stderr, "-n is not for execution; execution requires a DFA\n");
		return EXIT_FAILURE;
	}

	r = 0;

	if (argc != 1) {
		fprintf(stderr, "expected single string to match\n"); /* TODO: or filename */
		return EXIT_FAILURE;
	}

	r = !fsm_exec(fsm, fsm_sgetc, &argv[0]);

	fsm_free(fsm);

	return r;
}

