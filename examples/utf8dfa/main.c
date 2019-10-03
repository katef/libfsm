/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#define _POSIX_C_SOURCE 2

#include <unistd.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include <fsm/fsm.h>
#include <fsm/print.h>
#include <fsm/options.h>

static struct fsm_options opt;

static int
utf8(int cp, char c[])
{
	if (cp <= 0x7f) {
		c[0] =  cp;
		return 1;
	}

	if (cp <= 0x7ff) {
		c[0] = (cp >>  6) + 192;
		c[1] = (cp  & 63) + 128;
		return 2;
	}

	if (0xd800 <= cp && cp <= 0xdfff) {
		/* invalid */
		goto error;
	}

	if (cp <= 0xffff) {
		c[0] =  (cp >> 12) + 224;
		c[1] = ((cp >>  6) &  63) + 128;
		c[2] =  (cp  & 63) + 128;
		return 3;
	}

	if (cp <= 0x10ffff) {
		c[0] =  (cp >> 18) + 240;
		c[1] = ((cp >> 12) &  63) + 128;
		c[2] = ((cp >>  6) &  63) + 128;
		c[3] =  (cp  & 63) + 128;
		return 4;
	}

error:

	return 0;
}

static void
codepoint(struct fsm *fsm, int cp)
{
	fsm_state_t x, y;
	char c[4];
	int r, i;

	r = utf8(cp, c);
	if (!r) {
		fprintf(stderr, "codepoint out of range: U+%06X", (unsigned int) cp);
		exit(1);
	}

	(void) fsm_getstart(fsm, &x);

	/* TODO: construct trie, assuming increasing order */

	for (i = 0; i < r; i++) {
		if (!fsm_addstate(fsm, &y)) {
			perror("fsm_addstate");
			exit(1);
		}

		if (!fsm_addedge_literal(fsm, x, y, c[i])) {
			perror("fsm_addedge_literal");
			exit(1);
		}

		x = y;
	}

	fsm_setend(fsm, x, 1);
}

int
parse(const char *p, char **e)
{
	long l;

	errno = 0;
	l = strtol(p, e, 16);
	if ((l == LONG_MAX || l == LONG_MIN) && errno != 0) {
		fprintf(stderr, "parse error: %s\n", p);
		exit(1);
	}

	if (l < 0 || l > INT_MAX) {
		fprintf(stderr, "hex out of range: %s\n", p);
		exit(1);
	}

	return (int) l;
}

int
main(int argc, char *argv[])
{
	struct fsm *fsm;
	struct fsm_state *start;
	fsm_print *print;

	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;
	opt.always_hex        = 0;
	opt.comments          = 0;
	opt.case_ranges       = 0;

	print = fsm_print_api;

	{
		int c;

		while (c = getopt(argc, argv, "e:l:"), c != -1) {
			switch (c) {
			case 'e': opt.prefix = optarg; break;

			case 'l':
				if (strcmp(optarg, "dot") == 0) {
					print = fsm_print_dot;
				} else if (strcmp(optarg, "api") == 0) {
					print = fsm_print_api;
				} else if (strcmp(optarg, "c") == 0) {
					print = fsm_print_c;
				} else {
					fprintf(stderr, "Invalid language '%s'", optarg);
					exit(1);
				}
				break;

			case '?':
			default:
				return EXIT_FAILURE;
			}
		}

		argc -= optind;
		argv += optind;
	}

	if (argc > 0) {
		return EXIT_FAILURE;
	}

	fsm = fsm_new(&opt);
	if (fsm == NULL) {
		perror("fsm_new");
		exit(1);
	}

	start = fsm_addstate(fsm);
	if (start == NULL) {
		perror("fsm_addstate");
		exit(1);
	}

	fsm_setstart(fsm, start);

	for (;;) {
		char buf[16];
		buf[sizeof buf - 1] = 'x';
		char *e;
		int i, lo, hi;

		if (!fgets(buf, sizeof buf, stdin)) {
			break;
		}

		if (buf[sizeof buf - 1] == '\0' && buf[sizeof buf - 2] != '\n') {
			fprintf(stderr, "overflow\n");
			exit(1);
		}

		lo = parse(buf, &e);

		if (0 == strncmp(e, "..", 2)) {
			hi = parse(e + 2, &e);
		} else {
			hi = lo;
		}

		if (*e != '\n') {
			fprintf(stderr, "syntax error: %s\n", buf);
			exit(1);
		}

		for (i = lo; i <= hi; i++) {
			/* XXX: skip surrogates */
			if (0xd800 <= i && i <= 0xdfff) {
				continue;
			}

			codepoint(fsm, i);
		}
	}

	if (!fsm_minimise(fsm)) {
		perror("fsm_minimise");
		exit(1);
	}

	print(stdout, fsm);

	fsm_free(fsm);

	return 0;
}

