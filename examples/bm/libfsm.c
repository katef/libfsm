/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

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
	const static struct fsm_options opt_defaults;
	struct fsm_options opt = opt_defaults;
	char s[4096];
	const char *p;
	struct re_err e;

	opt.tidy = 0;
	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;
	opt.case_ranges = 1;
	opt.comments = 0;
	opt.io = FSM_IO_STR;

	p = s;
	fsm = re_comp(RE_PCRE, fsm_sgetc, &p, &opt, 0, &e);
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

	printf("int main(void) {\n");
	printf("\tchar s[4096];\n");
	printf("\tunsigned n;\n");
	printf("struct timespec pre, post;\n");
	printf("\n");

	printf("if (-1 == clock_gettime(CLOCK_MONOTONIC, &pre)) {\n");
	printf("	perror(\"clock_gettime\");\n");
	printf("	exit(EXIT_FAILURE);\n");
	printf("}\n");
	printf("\n");

	printf("\tn = 0;\n");
	printf("\twhile (fgets(s, sizeof s, stdin) != NULL) {\n");
	printf("\t\tn += fsm_main(s) != EOF;\n");
	printf("\t}\n");
	printf("\n");

	printf("if (-1 == clock_gettime(CLOCK_MONOTONIC, &post)) {\n");
	printf("	perror(\"clock_gettime\");\n");
	printf("	exit(EXIT_FAILURE);\n");
	printf("}\n");
	printf("\n");

/*
	printf("\tprintf(\"%%u %%s\\n\", n, n == 1 ? \"match\" : \"matches\");\n");
*/

	printf("{\n");
	printf("	double ms;\n");
	printf("\n");
	printf("	ms = 1000.0 * (post.tv_sec  - pre.tv_sec)\n");
	printf("				+ (post.tv_nsec - pre.tv_nsec) / 1e6;\n");
	printf("\n");
	printf("	printf(\"%%f\\n\", ms);\n");
	printf("}\n");
	printf("\n");

	printf("\treturn 0;\n");
	printf("}\n");

	fsm_free(fsm);

	return 0;
}

