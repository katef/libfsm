/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#define _POSIX_C_SOURCE 200112L

#include <unistd.h>

#include <stdio.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/print.h>
#include <fsm/options.h>

#include <re/re.h>

static struct fsm_options opt;

int main(int argc, char *argv[]) {
	struct fsm *fsm;
	struct fsm_state *start;
	char s[BUFSIZ];
	int (*dmf)(struct fsm *);
	void (*print)(FILE *, const struct fsm *);

	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;

	dmf = NULL;
	print = NULL;

	{
		int c;

		while (c = getopt(argc, argv, "h" "dmc"), c != -1) {
			switch (c) {
			case 'd': dmf = fsm_determinise; break;
			case 'm': dmf = fsm_minimise;    break;

			case 'c': print = fsm_print_dot; break;

			case 'h':
			case '?':
			default:
				goto usage;
			}
		}

		argc -= optind;
		argv += optind;
	}

	if (argc > 0) {
		goto usage;
	}

	fsm = fsm_new(&opt);
	if (fsm == NULL) {
		perror("fsm_new");
		return 1;
	}

	start = fsm_addstate(fsm);
	if (start == NULL) {
		perror("fsm_addtate");
		return 1;
	}

	while (fgets(s, sizeof s, stdin) != NULL) {
		struct fsm *r;
		struct re_err e;
		const char *p = s;
		struct fsm_state *rs;

		s[strcspn(s, "\n")] = '\0';

		r = re_comp(RE_LITERAL, fsm_sgetc, &p, &opt, 0, &e);
		if (r == NULL) {
			re_perror(RE_LITERAL, &e, NULL, s);
			return 1;
		}

		rs = fsm_getstart(r);

		fsm = fsm_merge(fsm, r);
		if (fsm == NULL) {
			perror("fsm_merge");
			return 1;
		}

		if (!fsm_addedge_epsilon(fsm, start, rs)) {
			perror("fsm_addedge_epsilon");
			return 1;
		}
	}

	fsm_setstart(fsm, start);

	if (dmf != NULL) {
		if (-1 == dmf(fsm)) {
			perror("fsm_determinise/minimise");
			return 1;
		}
	}

	if (print != NULL) {
		print(stdout, fsm);
	}

	return 0;

usage:

	fprintf(stderr, "usage: words [-dm]\n");
	fprintf(stderr, "       words -h\n");

	return 1;
}

