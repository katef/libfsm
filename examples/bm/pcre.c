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
	struct timespec pre, post;

	re = pcre_compile(argv[1], 0, &err, &o, NULL);
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


	if (-1 == clock_gettime(CLOCK_MONOTONIC, &pre)) {
		perror("clock_gettime");
		exit(EXIT_FAILURE);
	}

	while (fgets(s, sizeof s, stdin) != NULL) {
		r = pcre_exec(re, e, s, strlen(s) - 1, 0, 0, NULL, 0);
		if (r == PCRE_ERROR_NOMATCH) {
			continue;
		}

		if (r == 0) {
			n++;
			continue;
		}

		fprintf(stderr, "%d\n", r);
		abort();
	}

	if (-1 == clock_gettime(CLOCK_MONOTONIC, &post)) {
		perror("clock_gettime");
		exit(EXIT_FAILURE);
	}

	pcre_free(re);

/*
	printf("%u %s\n", n, n == 1 ? "match" : "matches");
*/

	{
		double ms;

		ms = 1000.0 * (post.tv_sec  - pre.tv_sec)
					+ (post.tv_nsec - pre.tv_nsec) / 1e6;

		printf("%f\n", ms);
	}

	return 0;
}

