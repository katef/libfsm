/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#define _POSIX_C_SOURCE 200112L

#include <unistd.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/print.h>
#include <fsm/options.h>

#include <re/re.h>
#include <re/strings.h>

static struct fsm_options opt;

int main(int argc, char *argv[]) {
	struct fsm *fsm;
	fsm_state_t start;
	char s[BUFSIZ];
	int (*dmf)(struct fsm *);
	void (*print)(FILE *, const struct fsm *);
	unsigned long ms, mt;
	int timing;
	int native = 0;
	int ahocorasick = 0;
	int unanchored = 0;
	struct re_strings *g;

	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;

	dmf = NULL;
	print = NULL;
	timing = 0;

	{
		int c;

		while (c = getopt(argc, argv, "h" "aNndmctf"), c != -1) {
			switch (c) {
			case 'a': ahocorasick = 1;       break;
			case 'd': dmf = fsm_determinise; break;
			case 'm': dmf = fsm_minimise;    break;

			case 'n': native = 1;            break;
			case 'N': unanchored = 1;        break;

			case 'c': print = fsm_print_dot; break;
			case 'f': print = fsm_print_fsm; break;

			case 't':
				timing = 1;
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

	if (argc > 0) {
		goto usage;
	}

	fsm = NULL;

	if (ahocorasick) {
		g = re_strings_new();
		if (g == NULL) {
			perror("re_strings_new");
			exit(EXIT_FAILURE);
		}
	} else {
		fsm = fsm_new(&opt);
		if (fsm == NULL) {
			perror("fsm_new");
			return 1;
		}

		if (!fsm_addstate(fsm, &start)) {
			perror("fsm_addtate");
			return 1;
		}
	}

	ms = 0;
	mt = 0;

	while (fgets(s, sizeof s, stdin) != NULL) {
		struct timespec pre, post;

		s[strcspn(s, "\n")] = '\0';

		if (*s == '\0') {
			continue;
		}

		if (-1 == clock_gettime(CLOCK_MONOTONIC, &pre)) {
			perror("clock_gettime");
			exit(EXIT_FAILURE);
		}

		if (ahocorasick) {
			if (!re_strings_add_str(g, s)) {
				perror("re_strings_add_str");
				exit(EXIT_FAILURE);
			}
		} else {
			fsm_state_t base_a, base_b;
			const char *p = s;
			struct re_err e;
			fsm_state_t rs;
			struct fsm *r;

			r = re_comp_new(native ? RE_NATIVE : RE_LITERAL, fsm_sgetc, &p, &opt, 0, &e);
			if (r == NULL) {
				re_perror(native ? RE_NATIVE : RE_LITERAL, &e, NULL, s);
				return 1;
			}

			(void) fsm_getstart(r, &rs);

			fsm = fsm_merge(fsm, r, &base_a, &base_b);
			if (fsm == NULL) {
				perror("fsm_merge");
				return 1;
			}

			start += base_a;
			rs    += base_b;

			if (!fsm_addedge_epsilon(fsm, start, rs)) {
				perror("fsm_addedge_epsilon");
				return 1;
			}
		}

		if (-1 == clock_gettime(CLOCK_MONOTONIC, &post)) {
			perror("clock_gettime");
			exit(EXIT_FAILURE);
		}

		ms += 1000 * (post.tv_sec - pre.tv_sec)
		           + ((long) post.tv_nsec - (long) pre.tv_nsec) / 1000000;
	}

	if (ahocorasick) {
		struct timespec pre, post;

		if (-1 == clock_gettime(CLOCK_MONOTONIC, &pre)) {
			perror("clock_gettime");
			exit(EXIT_FAILURE);
		}

		fsm = fsm_new(&opt);
		if (fsm == NULL) {
			perror("fsm_new");
			exit(EXIT_FAILURE);
		}

		if (!re_strings_build(fsm, g,
			unanchored ? 0 : (RE_STRINGS_ANCHOR_LEFT | RE_STRINGS_ANCHOR_RIGHT)))
		{
			perror("re_strings_builder_build");
			fsm_free(fsm);
			exit(EXIT_FAILURE);
		}

		if (-1 == clock_gettime(CLOCK_MONOTONIC, &post)) {
			perror("clock_gettime");
			exit(EXIT_FAILURE);
		}

		mt += 1000 * (post.tv_sec - pre.tv_sec)
		           + ((long) post.tv_nsec - (long) pre.tv_nsec) / 1000000;
	} else {
		fsm_setstart(fsm, start);
	}

	if (ahocorasick) {
		re_strings_free(g);
	}

	if (dmf != NULL) {
		struct timespec pre, post;

		if (-1 == clock_gettime(CLOCK_MONOTONIC, &pre)) {
			perror("clock_gettime");
			exit(EXIT_FAILURE);
		}

		if (-1 == dmf(fsm)) {
			perror("fsm_determinise/minimise");
			return 1;
		}

		if (-1 == clock_gettime(CLOCK_MONOTONIC, &post)) {
			perror("clock_gettime");
			exit(EXIT_FAILURE);
		}

		mt += 1000 * (post.tv_sec - pre.tv_sec)
		           + ((long) post.tv_nsec - (long) pre.tv_nsec) / 1000000;
	}

	if (print != NULL) {
		print(stdout, fsm);
	}

	if (timing) {
		printf("construction, reduction, total: %lu, %lu, %lu\n", ms, mt, ms + mt);
	}

	return 0;

usage:

	fprintf(stderr, "usage: words [-aNndmctf]\n");
	fprintf(stderr, "       words -h\n");

	return 1;
}

