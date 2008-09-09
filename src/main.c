/* $Id$ */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "syntax.h"
#include "fsm.h"
#include "out-dot.h"

void usage(void) {
	printf("usage: fsm [<input> [<output>]]\n");
}

FILE *xopen(int argc, char * const argv[], int i, FILE *f, const char *mode) {
	if (argc <= i || 0 == strcmp("-", argv[i])) {
		return f;
	}

	f = fopen(argv[i], mode);
	if (f == NULL) {
		perror(argv[i]);
		exit(EXIT_FAILURE);
	}

	return f;
}

int main(int argc, char *argv[]) {
	struct state_list *l;
	struct fsm_state *start;
	FILE *in;
	FILE *out;

	if (argc > 3) {
		usage();
		exit(EXIT_FAILURE);
	}

	in  = xopen(argc, argv, 1, stdin,  "r");
	out = xopen(argc, argv, 2, stdout, "w");

	parse(in, &l, &start);

	out_dot(out, l, start);

	/* TODO: free stuff. make an api.c */

	return 0;
}

