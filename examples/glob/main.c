/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

/*
 * An example illustrating glob(3)-like matching for strings by internally
 * constructing a DFA from the given globing string. A similar technique
 * could be used to implement regular expressions.
 *
 * The minimised DFA constructed is output to stdout, unless the -q option
 * is passed to quieten the output.
 *
 * Supported special characters are:
 *
 *  * - Zero or more occurrences of any character
 *  ? - Exactly one occurrence of any character.
 *
 * All other characters are verbatim. No escaping for special characters is
 * provided. A complete match is tested for; no trailing characters are
 * permitted.
 *
 * For example: ./glob "a??*b" "axxzzzb"
 *
 * Returns 0 on successful matching, 1 otherwise.
 */

#define _POSIX_C_SOURCE 2

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <unistd.h>

#include <fsm/fsm.h>
#include <fsm/pred.h>
#include <fsm/print.h>
#include <fsm/walk.h>

static void
usage(void)
{
	printf("usage: glob [-h] [-q] <glob> <string>\n");
}

static int
match(const struct fsm *fsm, const char *s)
{
	const struct fsm_state *state;

	assert(fsm != NULL);
	assert(fsm_all(fsm, fsm_isdfa));
	assert(s != NULL);

	if (1 != fsm_exec(fsm, fsm_sgetc, &s, &state)) {
		return 0;
	}

	assert(fsm_isend(fsm, state));

	return 1;
}

static struct fsm *
compile(const char *glob)
{
	struct fsm *fsm;
	struct fsm_state *state;
	const char *p;

	assert(glob != NULL);

	fsm = fsm_new(NULL);
	if (!fsm) {
		perror("fsm_new");
		exit(2);
	}

	state = fsm_addstate(fsm);
	if (!state) {
		perror("fsm_addstate");
		exit(2);
	}

	fsm_setstart(fsm, state);

	/* TODO: explain what the labels are */
	/* TODO: usage guide: you'd point to a struct for anything more complex */

	/*
	 * Here we construct an NFA built of each possible NFA fragment
	 * corresponding to the three different cases per input character. This is
	 * exactly equivalent to constructing an NFA out of fragments for a regular
	 * expression, though much more simple. See for example, "Modern Compiler
	 * Implementation in C", figure 2.6 which shows the NFA fragments required
	 * for a regular expression.
	 *
	 * The fragments for our possible globbing cases are:
	 *     a: 0 -> 1 "a";
	 *     ?: 0 -> 1 ?;
	 *     *: 0 -> 1 ?; 1 -> 0;
	 */
	for (p = glob; *p; p++) {
		struct fsm_state *new;

		/* TODO: we could omit this */
		new = fsm_addstate(fsm);
		if (!new) {
			perror("fsm_addstate");
			exit(2);
		}

		switch (*p) {
		case '?':
			if (!fsm_addedge_any(fsm, state, new)) {
				perror("fsm_addedge_any");
				exit(2);
			}
			break;

		case '*':
			if (!fsm_addedge_any(fsm, state, new)) {
				perror("fsm_addedge_any");
				exit(2);
			}

			if (!fsm_addedge_epsilon(fsm, new, state)) {
				perror("fsm_addedge_epsilon");
				exit(2);
			}
			new = state;
			break;

		default:
			if (!fsm_addedge_literal(fsm, state, new, *p)) {
				perror("fsm_addedge");
				exit(2);
			}
			break;
		}

		state = new;
	}

	fsm_setend(fsm, state, 1);

	return fsm;
}

int
main(int argc, char *argv[])
{
	struct fsm *fsm;
	int quiet = 0;
	int matched;

	{
		int c;

		while ((c = getopt(argc, argv, "hq")) != -1) {
			switch (c) {
			case 'h':
				usage();
				exit(0);

			case 'q':
				quiet = 1;
				break;

			case '?':
			default:
				usage();
				exit(2);
			}
		}

		argc -= optind;
		argv += optind;
	}

	if (argc != 2) {
		usage();
		exit(EXIT_FAILURE);
	}

	fsm = compile(argv[0]);
	assert(fsm != NULL);

	if (!fsm_minimise(fsm)) {
		exit(EXIT_FAILURE);
	}

	if (!quiet) {
		fsm_print_fsm(stdout, fsm);
	}

	matched = match(fsm, argv[1]);
	fsm_free(fsm);

	return !matched;
}

