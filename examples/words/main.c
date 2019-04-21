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

static struct fsm_options opt;

int main(int argc, char *argv[]) {
	struct fsm *fsm;
	struct fsm_state *start;
	char s[BUFSIZ];
	int (*dmf)(struct fsm *);
	void (*print)(FILE *, const struct fsm *);
	unsigned long ms, mt;
	int timing;
	int native = 0;
	int ahocorasick = 0;
	int unanchored = 0;
	int single_match = 1;

	struct {
		char **list;
		size_t len;
		size_t cap;
	} words = { NULL, 0, 0 };

	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;

	dmf = NULL;
	print = NULL;
	timing = 0;

	{
		int c;

		while (c = getopt(argc, argv, "h" "aNndmMctf"), c != -1) {
			switch (c) {
			case 'a': ahocorasick = 1;       break;
			case 'd': dmf = fsm_determinise; break;
			case 'm': dmf = fsm_minimise;    break;

			case 'M': single_match = 0;      break;
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
	start = NULL;

	if (ahocorasick) {
		words.cap = 1024;
		words.list = malloc(words.cap * sizeof(words.list[0]));
		if (words.list == NULL) {
			perror("allocating word list");
			exit(EXIT_FAILURE);
		}
	} else {
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
	}

	ms = 0;
	mt = 0;

	while (fgets(s, sizeof s, stdin) != NULL) {
		struct fsm *r;
		struct re_err e;
		const char *p = s;
		struct fsm_state *rs;
		struct timespec pre, post;

		s[strcspn(s, "\n")] = '\0';

		if (-1 == clock_gettime(CLOCK_MONOTONIC, &pre)) {
			perror("clock_gettime");
			exit(EXIT_FAILURE);
		}

		if (ahocorasick) {
			char *w;
			size_t wlen;

			if (words.len >= words.cap) {
				size_t newcap;
				char **wl;

				if (words.cap < 16384) {
					newcap = words.cap * 2;
				} else {
					newcap = words.cap + words.cap/4;
				}

				wl = malloc(newcap * sizeof wl[0]);
				if (wl == NULL) {
					perror("adding word");
					exit(EXIT_FAILURE);
				}

				fprintf(stderr, "words.len = %zu, words.cap = %zu, newcap = %zu, words.list=%p, wl=%p\n",
					words.len, words.cap, newcap, (void *)words.list, (void *)wl);
				memcpy(wl, words.list, words.len * sizeof words.list[0]);
				free(words.list);

				words.list = wl;
				words.cap = newcap;
			}

			wlen = strlen(p);
			w = malloc(wlen+1);
			if (w == NULL) {
				perror("adding word");
				exit(EXIT_FAILURE);
			}
			memcpy(w,p,wlen+1);

			words.list[words.len++] = w;
			continue;
		}

		r = re_comp(native ? RE_NATIVE : RE_LITERAL, fsm_sgetc, &p, &opt, 0, &e);
		if (r == NULL) {
			re_perror(native ? RE_NATIVE : RE_LITERAL, &e, NULL, s);
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

		fsm = re_strings(&opt, (const char **)words.list, words.len, unanchored ? 0 :  (RE_STRINGS_ANCHOR_LEFT | RE_STRINGS_ANCHOR_RIGHT));
		if (fsm == NULL) {
			perror("converting trie to fsm");
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

	fprintf(stderr, "usage: words [-dmct]\n");
	fprintf(stderr, "       words -h\n");

	return 1;
}

