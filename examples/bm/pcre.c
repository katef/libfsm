/*
 * Copyright 2019 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#define _POSIX_C_SOURCE 200112L

#include <unistd.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <pcre.h>

int
main(int argc, char *argv[])
{
	pcre *re;
	pcre_extra *e;
	int r;
	char s[4096];
	const char *err;
	int o;
	unsigned n;
	int flags;
	unsigned long ms;
	int i, max;

	max = BM_MAX;
	flags = 0;

	{
		int c;

		while (c = getopt(argc, argv, "h" "b"), c != -1) {
			switch (c) {
			case 'b':
				flags |= PCRE_ANCHORED;
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

	re = pcre_compile(argv[0], PCRE_ANCHORED, &err, &o, NULL);
	if (re == NULL) {
		fprintf(stderr, "pcre_compile: %s\n", err);
		abort();
	}

	e = pcre_study(re, 0, &err);
	if (e == NULL && err != NULL) {
		fprintf(stderr, "pcre_study: %s\n", err);
		abort();
	}

	n = 0;
	ms = 0;

	while (fgets(s, sizeof s, stdin) != NULL) {
		struct timespec pre, post;
		size_t z;

		z = strlen(s);

		if (-1 == clock_gettime(CLOCK_MONOTONIC, &pre)) {
			perror("clock_gettime");
			exit(EXIT_FAILURE);
		}

		for (i = 0; i < max; i++) {
			r = pcre_exec(re, e, s, z - 1, 0, 0, NULL, 0);
			if (r != 0 && r != PCRE_ERROR_NOMATCH) {
				fprintf(stderr, "%d\n", r);
				abort();
			}
		}

		if (-1 == clock_gettime(CLOCK_MONOTONIC, &post)) {
			perror("clock_gettime");
			exit(EXIT_FAILURE);
		}

		ms += 1000 * (long) (post.tv_sec - pre.tv_sec)
		           + ((long) post.tv_nsec - (long) pre.tv_nsec) / 1000000 / max;

		if (r == PCRE_ERROR_NOMATCH) {
			continue;
		}

		if (r == 0) {
			n++;
		}
	}

	pcre_free(re);

/*
	printf("%u %s\n", n, n == 1 ? "match" : "matches");
*/

	printf("%lu\n", ms);

	return 0;

usage:

	fprintf(stderr, "usage: pcre [-b] <regexp>\n");
	fprintf(stderr, "       pcre -h\n");

	return 1;
}

