/* $Id$ */

#include <stdio.h>

#include "syntax.h"
#include "fsm.h"
#include "out-dot.h"

int main(void) {
	struct state_list *l;
	struct fsm_state *start;

	parse(&l, &start);

	out_dot(stdout, start, l);

	/* TODO: free stuff. make an api.c */

	return 0;
}

