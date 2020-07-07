/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#define _POSIX_C_SOURCE 200112L

#include <unistd.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/options.h>
#include <fsm/print.h>

#include <re/re.h>

int
main(int argc, char *argv[])
{
	struct fsm *fsm;
	static const struct fsm_options opt_defaults;
	struct fsm_options opt = opt_defaults;
	char s[4096];
	const char *p;
	struct re_err e;
	enum re_flags flags;

	flags = 0;

	{
		int c;

		while (c = getopt(argc, argv, "h" "b"), c != -1) {
			switch (c) {
			case 'b':
				flags |= RE_ANCHORED;
				break;

			case 'h':
			case '?':
			default:
				goto usage;
			}
		}

		argc -= optind;
		argv += optind;
	}

	if (argc < 1) {
		goto usage;
	}

	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;
	opt.case_ranges = 1;
	opt.comments = 0;
	opt.io = FSM_IO_STR;

	p = argv[0];
	fsm = re_comp_new(RE_PCRE, fsm_sgetc, &p, &opt, flags, &e);
	if (fsm == NULL) {
		re_perror(RE_LITERAL, &e, NULL, s);
		return 1;
	}

	if (-1 == fsm_minimise(fsm)) {
		perror("fsm_minimise");
		return 1;
	}

	printf("#define _POSIX_C_SOURCE 200112L\n");
	printf("#define TOK_UNKNOWN EOF\n");
	printf("\n");
	printf("#include <unistd.h>\n");
	printf("#include <stdlib.h>\n");
	printf("#include <time.h>\n");
	printf("\n");

	fsm_print_c(stdout, fsm);

	printf("int\n");
	printf("main(void)\n");
	printf("{\n");
	printf("\tchar s[4096];\n");
	printf("\tunsigned n;\n");
	printf("\tunsigned long ms;\n");
	printf("\tint i, max;\n");
	printf("\n");

	printf("\tmax = %d;\n", BM_MAX);
	printf("\tms = 0;\n");
	printf("\tn = 0;\n");
	printf("\n");

	printf("\twhile (fgets(s, sizeof s, stdin) != NULL) {\n");
	printf("\t\tstruct timespec pre, post;\n");
	printf("\n");

	printf("\t\tif (-1 == clock_gettime(CLOCK_MONOTONIC, &pre)) {\n");
	printf("\t\t	perror(\"clock_gettime\");\n");
	printf("\t\t	exit(EXIT_FAILURE);\n");
	printf("\t\t}\n");
	printf("\n");

	printf("\t\tfor (i = 0; i < max; i++) {\n");
	printf("\t\t\tn += fsm_main(s) != EOF;\n");
	printf("\t\t}\n");
	printf("\n");

	printf("\t\tif (-1 == clock_gettime(CLOCK_MONOTONIC, &post)) {\n");
	printf("\t\t	perror(\"clock_gettime\");\n");
	printf("\t\t	exit(EXIT_FAILURE);\n");
	printf("\t\t}\n");
	printf("\n");

/*
	printf("\t\tprintf(\"%%u %%s\\n\", n, n == 1 ? \"match\" : \"matches\");\n");
*/

	printf("\t\tms += 1000.0 * (post.tv_sec  - pre.tv_sec)\n");
	printf("\t\t             + ((long) post.tv_nsec - (long) pre.tv_nsec) / 1000000 / %d;\n", BM_MAX);
	printf("\n");

	printf("\t}\n");
	printf("\n");

	printf("\tprintf(\"%%lu\\n\", ms);\n");
	printf("\n");

	printf("\treturn 0;\n");
	printf("}\n");

	fsm_free(fsm);

	return 0;

usage:

	fprintf(stderr, "usage: libfsm [-b] <regexp>\n");
	fprintf(stderr, "       libfsm -h\n");

	return 1;
}

