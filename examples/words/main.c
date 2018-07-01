/*
 * Copyright 2008-2017 Katherine Flavel
 *
 * See LICENCE for the full copyright terms.
 */

#include <stdio.h>
#include <string.h>

#include <fsm/fsm.h>
#include <fsm/bool.h>
#include <fsm/print.h>
#include <fsm/options.h>

#include <re/re.h>

static struct fsm_options opt;

int main(void) {
	struct fsm *fsm;
	struct fsm_state *start;
	char s[BUFSIZ];

	opt.anonymous_states  = 1;
	opt.consolidate_edges = 1;

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

	fsm_setstart(fsm, start);

	while (fgets(s, sizeof s, stdin) != NULL) {
		struct fsm *r;
		struct re_err e;
		const char *p = s;

		s[strcspn(s, "\n")] = '\0';

		fprintf(stderr, "%s\n", s);

		r = re_comp(RE_LITERAL, fsm_sgetc, &p, &opt, 0, &e);
		if (r == NULL) {
			re_perror(RE_LITERAL, &e, NULL, s);
			return 1;
		}

		fsm = fsm_union(fsm, r);
		if (fsm == NULL) {
			perror("fsm_union");
			return 1;
		}
	}

	if (-1 == fsm_minimise(fsm)) {
		perror("fsm_minimise");
		return 1;
	}

	fsm_print_dot(stdout, fsm);

	return 0;
}

